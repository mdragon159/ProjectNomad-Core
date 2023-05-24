#pragma once

#include "RollbackUser.h"
#include "Managers/RollbackTimeManager.h"
#include "Model/BaseSnapshot.h"
#include "Model/RollbackRuntimeState.h"
#include "Model/RollbackSettings.h"
#include "Network/P2PMessages/NetMessagesInput.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

// TODO: Local input delay (just separate and stagger render frame. Only real "confusion" is at verrry beginning to get the stagger)

namespace ProjectNomad {
    /**
    * Ingress point for all rollback behavior.
    * ie, this class encapsulates all necessary logic for rollback-related features.
    *
    * Also, useful high level general rollback logic outline: https://gist.github.com/rcmagic/f8d76bca32b5609e85ab156db38387e9
    * @tparam SnapshotType - defines struct used for frame snapshot. "Restoring" this should effectively return to a 
                             prior frame.
    **/
    template <typename SnapshotType>
    class RollbackManager {
        static_assert(std::is_base_of_v<BaseSnapshot, SnapshotType>, "SnapshotType must derive from BaseSnapshot");
        
      public:
        explicit RollbackManager(RollbackUser<SnapshotType>& rollbackUser) : mRollbackUser(rollbackUser) {}

        /**
        * Expected to be called at start of new game session before any other method is called.
        * @param rollbackSettings - settings for rollback behavior such as input delay
        **/
        void StartRollbackSession(const RollbackSettings& rollbackSettings) {
            if (mIsSessionRunning) {
                mLogger.LogWarnMessage("Start called while already running! Allowing but atm expected to end session first");
                EndRollbackSessionIfAny(); // Mainly to reset session running bool in case of setup failure later on
            }

            if (!AreSettingsValid(rollbackSettings)) {
                mLogger.LogErrorMessage("Provided settings are invalid!");
                return;
            }
            if (!TrySetupStateForSessionStart(rollbackSettings)) {
                mLogger.LogErrorMessage("State setup failed!");
                return;
            }

            mTimeManager.Start();
            mIsSessionRunning = true;
        }
        void EndRollbackSessionIfAny() {
            // Simply mark session as stopped running.
            // No need to clear existing data as all other public methods check for this explicitly
            mIsSessionRunning = false;
        }

        void OnReceivedTimeQualityReport(PlayerSpot remotePlayerSpot, FrameType remotePlayerFrame) {
            // Sanity checks
            if (!mIsSessionRunning) {
                mLogger.LogWarnMessage("Called while session not running!");
                return;
            }
            if (!mRollbackSettings.IsMultiplayerSession()) {
                mLogger.LogWarnMessage("Not in multiplayer session but getting called for some reason");
                return;
            }
            if (remotePlayerSpot == mRollbackSettings.localPlayerSpot) {
                mLogger.LogWarnMessage(
                    "Provided remote player spot is actually equal to local player spot! Player spot: " +
                    std::to_string(static_cast<int>(remotePlayerSpot))
                );
                return;
            }

            // Ignore any messages not from host player, as currently only time syncing to host.
            //      This is contrary to, say, typical 2-player fighting game rollback.  But this decision was made as
            //      there may be an arbitrary number of players and we need to time sync to a single point.
            //      Thus, the natural choice is to always time sync with the session host.
            if (remotePlayerSpot != mRollbackSettings.hostPlayerSpot) {
                return;
            }

            // Set TimeManager's time sync info. That's it for time sync management here! ^_^
            //      Note that we could also do input range validation, but the time manager call will also validate range for us.
            //      Not the best practice, but eh good enough for now.
            int64_t hostNumberOfFramesAhead = // Cast to int64 to avoid underflow as FrameType is currently uint32
                static_cast<int64_t>(remotePlayerFrame) - static_cast<int64_t>(mRuntimeState.lastProcessedFrame);
            mTimeManager.SetupTimeSyncForRemoteFrameDifference(mLogger, hostNumberOfFramesAhead);
        }
        void OnReceivedRemotePlayerInput(PlayerSpot remotePlayerSpot,
                                         FrameType updateFrame,
                                         const InputHistoryArray& playerInputs) {
            // Sanity checks
            if (!mIsSessionRunning) {
                mLogger.LogWarnMessage("Called while session not running!");
                return;
            }
            if (!mRollbackSettings.IsMultiplayerSession()) {
                mLogger.LogWarnMessage("Not in multiplayer session but getting called for some reason");
                return;
            }
            if (remotePlayerSpot == mRollbackSettings.localPlayerSpot) {
                mLogger.LogWarnMessage(
                    "Provided remote player spot is actually equal to local player spot! Player spot: " +
                    std::to_string(static_cast<int>(remotePlayerSpot))
                );
                return;
            }

            const FrameType preNewInputLastStoredFrame = mRuntimeState.inputManager.GetLastStoredFrameForPlayer(
                mLogger, remotePlayerSpot
            );
            
            // Determine number of new frames' inputs we've (supposedly) received
            FrameType numOfNewFrames = 0;
            // Edge case: At very beginning, we won't have any inputs stored and thus "last stored frame"
            //            will be the type's max value (as max value + 1 == 0 with unsigned int overflow)
            if (IsFrameValueMax(preNewInputLastStoredFrame)) {
                numOfNewFrames = updateFrame + 1; // +1 as "first" frame is frame 0. Update frame of frame 0 should still be 1 new frame
            }
            else {
                // Valid throwaway case: Check if we already have all the inputs for this packet.
                //      This may often occur due to inputs being sent via "UDP" (unreliable + unordered). Thus, we may
                //          receive a newer input packet before an older one (and hence the extra inputs per packet).
                //      Also 
                if (preNewInputLastStoredFrame >= updateFrame) {
                    mLogger.LogInfoMessage("TEMP: Ignoring message due to Point A");
                    return;
                }

                numOfNewFrames = updateFrame - preNewInputLastStoredFrame;
            }
            
            // Sanity check: Remote player should never send inputs from too far into future (as they should stall instead).
            //               ie, is number of new frames greater than # of inputs actually in the update message.
            if (numOfNewFrames > kInputsHistorySize) {
                mLogger.LogWarnMessage(
                    "Ignoring, possible bad update as further ahead into future than expected! Player spot: " +
                    std::to_string(static_cast<int>(remotePlayerSpot)) + ", previous last frame stored: " +
                    std::to_string(preNewInputLastStoredFrame) + ", received update frame: " +
                    std::to_string(updateFrame)
                );
                return;
            }

            // Finally add the new frames one by one, from oldest to newest.
            //      Note atm that input data storage expects inputs to be incrementally added one by one.
            for (FrameType count = 1; count <= numOfNewFrames; count++) { // Start at 1 instead of 0 to simplify math
                FrameType targetFrame = preNewInputLastStoredFrame + count;

                // Retrieve the appropriate input for this frame.
                //      Note that the "head" (0th index) is the latest input.
                FrameType targetIndex = numOfNewFrames - count; // Validated size earlier. Should be in range of 0 to kInputsHistorySize - 1
                const CharacterInput& newInput = playerInputs.at(targetIndex);
                
                mRuntimeState.inputManager.SetInputForPlayer(
                    mLogger, targetFrame, remotePlayerSpot, newInput
                );
            }
        }

        void PauseGame() {
            // Sanity checks
            if (!mIsSessionRunning) {
                mLogger.LogWarnMessage("Called while session not running!");
                return;
            }
            if (mTimeManager.IsPaused()) {
                mLogger.LogWarnMessage("Ignoring as called while already paused");
                return;
            }
            if (mRollbackSettings.IsMultiplayerSession()) { // Theoretically could support multiplayer pausing but needs much more work
                mLogger.LogWarnMessage("Cannot pause in a multiplayer session!");
                return;
            }

            mTimeManager.Pause();
        }
        void ResumeGame() {
            // Sanity checks
            if (!mIsSessionRunning) {
                mLogger.LogWarnMessage("Called while session not running!");
                return;
            }
            if (!mTimeManager.IsPaused()) {
                mLogger.LogWarnMessage("Ignoring as called while not actually paused");
                return;
            }

            mTimeManager.Resume();
        }

        /**
        * Expected to be called every frame regardless of whether or not gameplay is running, as may have network
        * related behavior to handle before actual game start.
        * @returns number of new gameplay frames processed, if any. Intended for higher level usage, such as perhaps
        *           passing this back to rendering side to smoothen camera or such.
        **/
        FrameType OnTick() {
            // If not running, then nothing to do
            if (!mIsSessionRunning) {
                return 0;
            }
            // If paused, then nothing to do as wel
            if (mTimeManager.IsPaused()) {
                if (mRollbackSettings.IsMultiplayerSession()) { // Extra sanity check. Expected to never happen atm
                    mLogger.LogWarnMessage("Game currently paused but no support for multiplayer pausing atm!");
                }
                return 0;
            }
            
            // TODO: Check if should rollback (and only update rendering once even with sync test)
            bool didRollbackOccur = false;
            
            // Do normal processing for x number of frames (time based)
            FrameType numOfNewFramesToProcess = mTimeManager.CheckHowManyFramesToProcess();
            for (FrameType i = 0; i < numOfNewFramesToProcess; i++) {
                // Edge case: If we don't have enough inputs for a frame update, then wait for inputs.
                //      FUTURE: If stalled for inputs, then make TimeManager return 1 on next call to CheckHowManyFramesToProcess()
                //              This would assure we don't have a full frame time's worth of delay regardless of when
                //              the actual input comes in.
                //              (ie, if input comes in immediately then we still need to wait a frame's worth of time to process)
                RollbackStallInfo stallInfo = CheckIfShouldStallForRemoteInputs();
                if (stallInfo.shouldStall) {
                    mRollbackUser.OnStallingForRemoteInputs(stallInfo);
                    
                    numOfNewFramesToProcess = i; // Update number of frames we actually processed to be accurate for latter logic
                    break; //  Stop trying to process any frames or otherwise do any further updates here
                }

                // Try to retrieve input for this frame, if any
                FrameType targetFrame = mRuntimeState.lastProcessedFrame + 1;
                CharacterInput localPlayerInput = {};
                bool wasAnyInputGiven = mRollbackUser.GetInputForNextFrame(targetFrame, localPlayerInput);
                // Edge case: If no input given, then "user" doesn't want us to process any more frames.
                //            Such as if this was using a replay file and the replay ran out of inputs
                if (!wasAnyInputGiven) {
                    numOfNewFramesToProcess = i; // Update number of frames we actually processed to be accurate for latter logic
                    break; //  Stop trying to process any frames or otherwise do any further updates here
                }
                
                // Store input for given frame so can be retrieved as needed later on
                mRuntimeState.inputManager.SetInputForPlayer(
                    mLogger, targetFrame, mRollbackSettings.localPlayerSpot, localPlayerInput
                );

                // Actually do the processing for the "next" frame.
                //      This is a generic method reused for "processing one more frame" during other contexts like
                //      post-rollback re-processing of frames.
                ProcessNextFrame(false, didRollbackOccur);

                // TODO: Post-processing related to frame inputs exiting rollback window (eg, for replays or checksums)
                //       Note that can either be done here OR all at once afterwards, depending on how long input
                //       storage is keeping the old inputs. Need to double check and assure that
                // See old method
                //      FUTURE: Rewrite stuff below so...  neater I guess? Not satisfied with how messy this feels, and
                //                  non-robust this "feels" (lotsa overlapping expectations, like relying on fact that
                //                  this is only place that we actually first move lastProcessedFrame to higher values
                //                  for first time, as rollback doesn't increase frame count compared to pre-rollback).
                if (mRuntimeState.lastProcessedFrame % RollbackStaticSettings::kTimeQualityReportFrequencyInFrames == 0) {
                    mRollbackUser.SendTimeQualityReport(mRuntimeState.lastProcessedFrame);
                }
            }

            // If actually processed any new frames...
            if (numOfNewFramesToProcess > 0) {
                // Handle SyncTest *after* normal processing is done so we can redo the frames again and compare results
                if (mRollbackSettings.useSyncTest) {
                    HandleSyncTest();
                    
                    // FUTURE: Mark didRollbackOccur = true so do PostRollback call as well (for a full render re-sync).
                    //         However, not strictly necessary for sync test since should have same results
                    //         before vs after outside of bugs.
                }

                // If playing multiplayer, then send (new) local player inputs to other players in session
                if (mRollbackSettings.IsMultiplayerSession()) {
                    // Since packets are sent via "UDP" (unreliable unordered), add many inputs per update packet.
                    // In future, we only want to send a certain number of inputs to reduce packet size.
                    // However for initial implementation simplicity, just always send many inputs at once.

                    // Fill in inputs from newest to oldest (so index 0 is newest input)
                    InputHistoryArray latestInputs = {};
                    for (FrameType i = 0; i < kInputsHistorySize; i++) {
                        FrameType targetFrame = mRuntimeState.lastProcessedFrame - i;

                        const CharacterInput& localPlayerInput = mRuntimeState.inputManager.GetPlayerInputForFrame(
                            mLogger, targetFrame, mRollbackSettings.localPlayerSpot
                        );
                        latestInputs[i] = localPlayerInput;

                        // Edge case: If run out of inputs to send (like first few frames of gameplay), then stop
                        if (targetFrame == 0) {
                            break;
                        }
                    }
                    
                    mRollbackUser.SendLocalInputsToRemotePlayers(mRuntimeState.lastProcessedFrame, latestInputs);
                }

                // Notify user of confirmed inputs (such as to help decide when to flush inputs to a replay file)
                FrameType curLocalRollbackRange = GetCurrentMaxPossibleRollbackFrames();
                if (curLocalRollbackRange > 0) { // Is rollback possible at all?
                    if (mRuntimeState.lastProcessedFrame >= curLocalRollbackRange) { // If beyond "initial" frames, then have frames we can confirm
                        // For simplified logic, just validate any frame once it exits the possible rollback range.
                        // (Technically we could consider inputs never changing for sync test and whatnot, but no need to complicate further)
                        mRollbackUser.OnInputsExitRollbackWindow(mRuntimeState.lastProcessedFrame - curLocalRollbackRange);
                    }
                }
                else {
                    mRollbackUser.OnInputsExitRollbackWindow(mRuntimeState.lastProcessedFrame);
                }
            }

            // If rollback occurred, then we've been calling the non-rendering RollbackUser frame update call. Now that
            // rollback is over, we should let User know that it's time to update rendering
            if (didRollbackOccur) {
                mRollbackUser.OnPostRollback();
            }

            return numOfNewFramesToProcess;
        }
        
        /**
        * Creates a snapshot to restore the internal state of the manager itself.
        * Useful for restoring snapshots outside of rollback process itself, such as during advanced offline replay
        * playback features.
        *
        * Note that this is necessary as restoring "arbitrary" snapshots may result in restoring state outside of the
        * ordinary rollback window.
        **/
        const RollbackRuntimeState<SnapshotType>& GetInternalStateSnapshot() const {
            return mRuntimeState;
        }
        /**
        * Restores internal state of this manager.
        * NOTE: This should be NEVER used during multiplayer matches, as may throw out remote match tracking.
        *       Eg, may throw out inputs received from remote players.
        * This is only intended to be used for special local-only features, such as arbitrary at-any-time snapshot
        *       restoration (akin to save state cheats) or advanced replay playback seeking/skipping time features.
        * @param snapshot - Snapshot to restore
        **/
        void RestoreInternalStateSnapshot(const RollbackRuntimeState<SnapshotType>& snapshot) {
            // FUTURE: Maybe explicitly check that not in multiplayer session?
            
            mRuntimeState = snapshot;
        }

        // Technically could just get internal state snapshot and retrieve this directly, but nice not to care so
        //      much about the internal snapshot implementation and instead let RollbackManager directly support this.
        const SnapshotType& GetLatestFrameSnapshot() const {
            return mRuntimeState.snapshotManager.GetLatestFrameSnapshot();
        }

      private:
        bool AreSettingsValid(const RollbackSettings& rollbackSettings) const {
            if (PlayerSpotHelpers::IsInvalidTotalPlayers(rollbackSettings.totalPlayers)) {
                mLogger.LogWarnMessage("Invalid total players! Provided: " + std::to_string(rollbackSettings.totalPlayers));
                return false;
            }
            if (PlayerSpotHelpers::IsPlayerSpotOutsideTotalPlayers(rollbackSettings.totalPlayers, rollbackSettings.localPlayerSpot)) {
                mLogger.LogWarnMessage(
                     "Invalid local player spot! Provided total players: " + std::to_string(rollbackSettings.totalPlayers) +
                     ", provided player spot: " + std::to_string(static_cast<int>(rollbackSettings.localPlayerSpot))
                 );
                return false;
            }

            // If using sync test, then assure its settings are valid
            if (rollbackSettings.useSyncTest) {
                if (rollbackSettings.syncTestFrames == 0
                    || rollbackSettings.syncTestFrames > RollbackStaticSettings::kMaxRollbackFrames) {
                    mLogger.LogWarnMessage(
                        "Provided sync test frames is outside expected range. Provided: " +
                        std::to_string(rollbackSettings.syncTestFrames)
                    );
                    return false;
                }
            }

            // Assure that input delay is not outside expected range
            auto localInputDelayMagnitude = static_cast<FrameType>(std::abs(rollbackSettings.localInputDelay));
            if (localInputDelayMagnitude > RollbackStaticSettings::kMaxInputDelay) {
                mLogger.LogWarnMessage("Provided input delay is outside expected window: " + std::to_string(rollbackSettings.localInputDelay));
                return false;
            }

            // Multiplayer only checks
            if (rollbackSettings.IsMultiplayerSession()) {
                if (rollbackSettings.localInputDelay < 0) {
                    mLogger.LogWarnMessage(
                        "Negative input delay not valid while in online multiplayer! Provided value: "
                        + std::to_string(rollbackSettings.localInputDelay)
                    );
                    return false;
                }
                
                if (PlayerSpotHelpers::IsPlayerSpotOutsideTotalPlayers(rollbackSettings.totalPlayers, rollbackSettings.hostPlayerSpot)) {
                    mLogger.LogWarnMessage(
                        "Invalid host player spot! Provided total players: " + std::to_string(rollbackSettings.totalPlayers) +
                        ", provided player spot: " + std::to_string(static_cast<int>(rollbackSettings.hostPlayerSpot))
                    );
                    return false;
                }
            }
            // Single player only checks
            else {
                if (rollbackSettings.localPlayerSpot != PlayerSpot::Player1) {
                    mLogger.LogWarnMessage(
                        "Single player mode but provided non-Player1 spot! Provided value: "
                        + std::to_string(static_cast<int>(rollbackSettings.localPlayerSpot))
                    );
                    return false;
                }
            }
            
            return true;
        }
        bool TrySetupStateForSessionStart(const RollbackSettings& rollbackSettings) {
            // Reset any necessary runtime state, such as 
            mRuntimeState = {};
            // Store the session info as a whole for direct reference at any time
            mRollbackSettings = rollbackSettings;

            // Setup relevant managers
            if (!mRuntimeState.snapshotManager.OnSessionStart()) {
                mLogger.LogWarnMessage("Snapshot manager setup failed!");
                return false;
            }
            if (!mRuntimeState.inputManager.SetupForNewSession(mLogger, rollbackSettings)) {
                mLogger.LogWarnMessage("Input manager setup failed!");
                return false;
            }
            
            return true;
        }

        /**
        * Actually do gameplay update for next frame
        * @param skipSnapshotCreation - true if should skip snapshot creation, such as when re-processing the first
        *                               frame of a series since the snapshot would be identical to what's stored
        * @param didRollbackOccur - true if rollback already occurred in this manager update
        **/
        void ProcessNextFrame(bool skipSnapshotCreation, bool didRollbackOccur) {
            FrameType targetFrame = mRuntimeState.lastProcessedFrame + 1; // Don't take this as an input to assure we always do the "next" frame

            if (!skipSnapshotCreation) {
                // Store game state at START of frame.
                // This works around how to re-process first (0th) frame and thus how to rollback to before then
                StoreSnapshot(targetFrame);
            }
            
            // Grab input(s) for updating game
            PlayerInputsForFrame inputsForFrame = mRuntimeState.inputManager.GetInputsForFrame(mLogger, targetFrame);

            // Update game. Note that this is also expected to increment RollbackUser's frame tracking as well
            if (!didRollbackOccur) {
                mRollbackUser.ProcessFrame(targetFrame, inputsForFrame);
            }
            else {
                mRollbackUser.ProcessFrameWithoutRendering(targetFrame, inputsForFrame);
            }

            // Finally internally remember that we processed this frame
            mRuntimeState.lastProcessedFrame = targetFrame;
        }

        void HandleSyncTest() {
            // Redundant sanity check
            if (mRollbackSettings.syncTestFrames == 0) {
                mLogger.LogWarnMessage("RollbackManager::HandleSyncTest", "Rollback frame amount is 0!");
                return;
            }
            // If in initial frames - so can't rollback far enough - then don't bother testing
            if (mRuntimeState.lastProcessedFrame <  mRollbackSettings.syncTestFrames) {
                return;
            }
            
            // Grab current snapshot's checksum so we know what to compare against
            uint32_t preTestSnapshotChecksum = mRuntimeState.snapshotManager.GetSnapshot(mRuntimeState.lastProcessedFrame).CalculateChecksum();
            
            // Do normal rollback process
            // Note that OnFixedGameplayUpdate() doesn't care that we call HandleRollback here as we expect no different
            // state and thus no difference in visuals
            FrameType firstFrameToReprocess = mRuntimeState.lastProcessedFrame - mRollbackSettings.syncTestFrames;
            HandleRollback(firstFrameToReprocess);

            // Finally compare hashes and output result
            uint32_t postTestSnapshotChecksum = mRuntimeState.snapshotManager.GetSnapshot(mRuntimeState.lastProcessedFrame).CalculateChecksum();
            if (preTestSnapshotChecksum != postTestSnapshotChecksum) {
                mLogger.LogWarnMessage(
                    "RollbackManager::HandleSyncTest",
                    "SyncTest failed for frame " + std::to_string(mRuntimeState.lastProcessedFrame)
                );
            }

            // Optionally log additional info to help with arbitrary debugging (as ended up adding this log statement multiple times)
            if (mRollbackSettings.logSyncTestChecksums) {
                mLogger.LogInfoMessage(
                    "RollbackManager::HandleSyncTest",
                    "Checksum Pre: " + std::to_string(preTestSnapshotChecksum) +
                    " | Post: " + std::to_string(postTestSnapshotChecksum)
                );
            }
        }

        /**
        * Handles process of rolling back then re-processing relevant frames. Relevant inputs are expected to be stored
        * before calling this method.
        * @param firstFrameToReprocess - what frame was incorrect and thus needs to start re-processing from
        **/
        void HandleRollback(FrameType firstFrameToReprocess) {
            // Calculate number of frames we actually need to re-process
            // Eg: Last processed frame = 2, input = 0, so we want to redo 3 frames (0 through 2)
            const FrameType numOfFramesToProcess = mRuntimeState.lastProcessedFrame - firstFrameToReprocess + 1;

            // Sanity check input
            if (firstFrameToReprocess > mRuntimeState.lastProcessedFrame) { // Should never occur during normal play (incl overflow)
                mLogger.LogErrorMessage(
                    "RollbackManager::HandleRollback",
                    "Non-existent frame to re-process! Last processed frame: " + std::to_string(mRuntimeState.lastProcessedFrame) +
                    ", input frame: " + std::to_string(firstFrameToReprocess)
                );
                return;
            }
            if (numOfFramesToProcess > RollbackStaticSettings::kMaxRollbackFrames) {
                mLogger.LogErrorMessage(
                    "RollbackManager::HandleRollback",
                    "Trying to rollback beyond supported window! Last processed frame: " +
                    std::to_string(mRuntimeState.lastProcessedFrame) + ", input frame: " + std::to_string(firstFrameToReprocess)
                );
                return;
            }
            
            // 1. Restore snapshot before the first frame we want to reprocess
            RestoreSnapshot(firstFrameToReprocess);
            
            // 2. Simply reprocess all needed frames
            for (FrameType i = 0; i < numOfFramesToProcess; i++) {
                // Skip snapshot creation for first frame as it'll be identical to what's already stored and been restored,
                // since snapshots are stored at the start of a frame
                bool skipSnapshotCreation = i == 0;
                ProcessNextFrame(skipSnapshotCreation, true);
            }
        }

        /**
        * Generates and stores a snapshot so we can re-process the target frame if/when necessary
        * @param targetFrame - intended frame to generate snapshot for. This should be BEFORE frame is processed.
        **/
        void StoreSnapshot(FrameType targetFrame) {
            // Likely redundant sanity checks BUT could uncover bugs in future!
            if (IsFrameValueMax(targetFrame)) {
                mLogger.LogErrorMessage(
                    "RollbackManager::StoreSnapshot",
                    "Invalid frame (max value) to store snapshot for!: Input frame: " + std::to_string(targetFrame)
                );
                return;
            }
            if (targetFrame > mRuntimeState.lastProcessedFrame + 1) {
                mLogger.LogErrorMessage(
                    "RollbackManager::StoreSnapshot",
                    "Trying to store snapshot for unprocessed frame! Last processed frame: " +
                    std::to_string(mRuntimeState.lastProcessedFrame) + ", input frame: " + std::to_string(targetFrame)
                );
                return;
            }

            {   // Create then swap-replace insert snapshot.
                //      Using scope delimiters to make explicit that variable should NOT be used after this cuz of the
                //      store call using swap-replace.
                SnapshotType snapshot = {}; 
                mRollbackUser.GenerateSnapshot(targetFrame, snapshot);
                mRuntimeState.snapshotManager.StoreSnapshot(targetFrame, snapshot); 
            }

            if (mRollbackSettings.logChecksumForEveryStoredFrameSnapshot) {
                uint32_t curSnapshotChecksum = mRuntimeState.snapshotManager.GetSnapshot(targetFrame).CalculateChecksum();
                mLogger.LogInfoMessage(
                    "RollbackManager::StoreSnapshot",
                    "Frame " + std::to_string(targetFrame) + ": " + std::to_string(curSnapshotChecksum)
                );
            }
        }
        void RestoreSnapshot(FrameType frameToReprocess) {
            // Likely redundant sanity checks BUT could uncover bugs in future!
            if (IsFrameValueMax(frameToReprocess)) {
                mLogger.LogErrorMessage(
                    "RollbackManager::RestoreSnapshot",
                    "Invalid frame (max value) to retrieve snapshot for!: Input frame: " + std::to_string(frameToReprocess)
                );
                return;
            }
            if (frameToReprocess > mRuntimeState.lastProcessedFrame) { // Snapshots are expected to be generated BEFORE an update occurs
                mLogger.LogErrorMessage(
                    "RollbackManager::RestoreSnapshot",
                    "Trying to retrieve snapshot for unprocessed frame! Last processed frame: " +
                    std::to_string(mRuntimeState.lastProcessedFrame) + ", input frame: " + std::to_string(frameToReprocess)
                );
                return;
            }
            
            // Get and restore game snapshot
            const SnapshotType& snapshot = mRuntimeState.snapshotManager.GetSnapshot(frameToReprocess);
            mRollbackUser.RestoreSnapshot(frameToReprocess, snapshot);

            // Also update necessary internal state, which is just this manager's frame tracking at the moment.
            //  Yes, at time of writing ALL other rollback state is either history based (so shouldn't be overwritten)
            //  or session based (so no need to restore)
            mRuntimeState.lastProcessedFrame = frameToReprocess - 1;
        }

        // Called to do one-time processing after any new gameplay  frames
        void DoPostNewGameplayFramesProcessing() {
            // Sanity check
            if (IsFrameValueMax(mRuntimeState.lastProcessedFrame)) {
                mLogger.LogWarnMessage(
                    "Current frame value is still at max after gameplay update supposedly occurred!"
                );
                return;
            }

            FrameType curLocalRollbackRange = GetCurrentMaxPossibleRollbackFrames();
            if (curLocalRollbackRange > 0) { // Using local rollback at all?
                if (mRuntimeState.lastProcessedFrame >= curLocalRollbackRange) { // If beyond "initial" frames, then have frames we can confirm
                    // For simplified logic, just validate any frame once it exits the possible rollback range.
                    // (Technically we could consider inputs never changing for synctest and whatnot, but no need to complicate further)
                    mRollbackUser.OnInputsExitRollbackWindow(mRuntimeState.lastProcessedFrame - curLocalRollbackRange);
                }
            }
            // Otherwise not using any form of rollback at all in the current session, so immediately validate every frame
            else {
                mRollbackUser.OnInputsExitRollbackWindow(mRuntimeState.lastProcessedFrame);
            }
        }

        FrameType GetCurrentMaxPossibleRollbackFrames() const {
            if (mRollbackSettings.IsMultiplayerSession()) {
                return RollbackStaticSettings::kMaxRollbackFrames;
            }
            if (mRollbackSettings.useSyncTest) {
                return mRollbackSettings.syncTestFrames;
            }

            return 0;
        }

        /**
        * Checks if the given frame is at the max possible value.
        * The underlying intention here is to easily identify what code is relying on this assumption (mostly for sanity checks).
        * We'll need to review these use cases in future to see if super long games with frame count overflow are feasible.
        * (For now, don't think are feasible cuz 4,294,967,295 - current FrameType max value - is hundreds of days long with 60fps!)
        * @param frame - Frame value to check
        * @returns true if frame value is at the max possible value for the numeric type
        **/
        static bool IsFrameValueMax(FrameType frame) {
            return frame == std::numeric_limits<FrameType>::max();
        }

        RollbackStallInfo CheckIfShouldStallForRemoteInputs() const {
            if (!mRollbackSettings.IsMultiplayerSession()) { // Stalling is only done during multiplayer games
                return RollbackStallInfo::NoStall();
            }

            // Check if missing too many inputs for any players
            std::vector<PlayerSpot> waitingOnPlayerSpots = {};
            FrameType targetFrame = mRuntimeState.lastProcessedFrame + 1;
            bool needToWaitOnAnyPlayerInputs = mRuntimeState.inputManager.IsFrameOutsideOfGetRangeForAnyPlayer(
                mLogger, targetFrame, waitingOnPlayerSpots
            );

            if (!needToWaitOnAnyPlayerInputs) {
                return RollbackStallInfo::NoStall();
            }

            // Sanity check result size (as this is a hard assumption for next logic)
            if (waitingOnPlayerSpots.size() > FlexStallPlayerInfoArray::GetMaxSize()) {
                mLogger.LogWarnMessage(
                    "Waiting on player spots result size is larger than max expected size! Max possible size: "
                    + std::to_string(FlexStallPlayerInfoArray::GetMaxSize()) + ", result waiting on players size: "
                    + std::to_string(waitingOnPlayerSpots.size())
                );
                return RollbackStallInfo::WithStall({}); // Make sure to stall as continuing could cause downstream issues
            }

            // Get bit more info on each player we're waiting on then return that info
            FlexStallPlayerInfoArray waitingOnPlayersFullInfo = {};
            for (PlayerSpot playerSpot : waitingOnPlayerSpots) {
                FrameType lastReceivedFrame = mRuntimeState.inputManager.GetLastStoredFrameForPlayer(
                    mLogger, playerSpot
                );
                waitingOnPlayersFullInfo.Add({playerSpot, lastReceivedFrame});
            }
            
            return RollbackStallInfo::WithStall(waitingOnPlayersFullInfo);
        }
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();
        RollbackUser<SnapshotType>& mRollbackUser;

        RollbackSettings mRollbackSettings = {};
        
        bool mIsSessionRunning = false;
        RollbackTimeManager mTimeManager = {}; // Assuming no need to be in rollback-able runtime state atm, including pausing + resuming
        RollbackRuntimeState<SnapshotType> mRuntimeState = {};
    };
}
