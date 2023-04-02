#pragma once

#include "RollbackInputManager.h"
#include "RollbackSnapshotManager.h"
#include "Interface/RollbackUser.h"
#include "Model/BaseSnapshot.h"
#include "Model/RollbackManagerSnapshot.h"
#include "Model/RollbackSessionInfo.h"
#include "Model/RollbackSettings.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

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
        RollbackManager(RollbackUser<SnapshotType>& rollbackUser, RollbackSnapshotManager<SnapshotType>& snapshotManager)
        : mRollbackUser(rollbackUser), mSnapshotManager(snapshotManager) {}

        /**
        * Expected to be called at start of new game session before any other method is called.
        * @param rollbackSettings - settings for rollback behavior such as input delay
        * @param rollbackSessionInfo - session-specific settings such as number of players
        **/
        void StartRollbackSession(RollbackSettings rollbackSettings, RollbackSessionInfo rollbackSessionInfo) {
            if (mIsSessionRunning) {
                mLogger.logWarnMessage("RollbackManager::OnSessionStart", "Start called while already running!");
            }

            AssureSettingsAreValid(rollbackSettings, rollbackSessionInfo);
            
            SetupStateForSessionStart(rollbackSettings, rollbackSessionInfo);
            mInputManager.OnSessionStart(rollbackSettings, rollbackSessionInfo);
            mSnapshotManager.OnSessionStart();
            
            mIsSessionRunning = true;
        }
        
        void EndRollbackSession() {
            // Simply mark session as stopped running.
            // No need to clear existing data as all other public methods check for this explicitly
            mIsSessionRunning = false;
        }

        /**
        * Expected to be called on every "tick". Handles ordinary
        * @param isFixedGameplayFrame - true if called on the fixed gameplay framerate (eg, during a 60fps frame for a 60fps game)
        * @param currentFrame - Current frame
        * @param localPlayerInput - 
        **/
        void OnUpdate(bool isFixedGameplayFrame, FrameType currentFrame, const CharacterInput& localPlayerInput) {
            if (!mIsSessionRunning) {
                mLogger.logWarnMessage("RollbackManager::OnFixedUpdate", "No running session");
                return;
            }
            // Sanity check: Verify that the frame is expected, which should always be called one frame forward
            if (currentFrame != mLastProcessedFrame + 1) {
                mLogger.logErrorMessage(
                    "RollbackManager::OnUpdate",
                    "Received unexpected frame which should be one greater than last processed frame! Expected frame: "
                    + std::to_string(mLastProcessedFrame + 1) + ", provided frame: " + std::to_string(currentFrame)
                );
                return; // Don't try to proceed so frame drift is as noticeable as possible
            }

            CheckAndHandleRollbackForNetworkedMPSession();

            // If using negative input delay but haven't yet processed initial prediction frames...
            if (mIsUsingLocalNegativeInputDelay && currentFrame == 0) {
                // Run simulation for those initial frames so we're actually AHEAD of the provided input (which
                // InputManager actually handles)
                for (FrameType i = 0; i < mLocalPredictionAmount; i++) {
                    OnFixedGameplayUpdate(localPlayerInput);
                }
            }
            
            // Any further work is only appropriate for fixed gameplay frames
            if (isFixedGameplayFrame) {
                OnFixedGameplayUpdate(localPlayerInput);
            }
        }

        /**
        * Expected to be called at the end of every tick.
        * Explicit call is necessary to prevent extra work from being done when multiple updates need to occur back to back.
        * (ie, if local player's frame rate is too low and thus need to catch up, we don't want to do extraneous work)
        * @param didAnyLocalFixedGameplayFramesOccur true if OnUpdate was called with first param as true, false otherwise
        **/
        void OnPostUpdate(bool didAnyLocalFixedGameplayFramesOccur) {
            // Sanity check
            if (!mIsSessionRunning) {
                mLogger.logWarnMessage("RollbackManager::OnPostUpdate", "No running session");
                return;
            }

            CheckAndNotifyConfirmedFrameCount(didAnyLocalFixedGameplayFramesOccur);
        }

        /*void OnReplayDrivenFixedUpdate(FrameType newFrame,
                                        const std::initializer_list<const PlayerInput&>& allPlayerInputsForFrame) {
            if (!mIsSessionRunning) {
                mLogger.logWarnMessage("RollbackManager::OnReplayDrivenFixedUpdate", "No running session");
                return;
            }
            
            // Just do simplest level of logic:
            // 1. Store inputs
            // 2. Call game update
            // 3. Store snapshot
        }*/

        /*void OnReceivedRemovePlayerInput(const PlayerId& remotePlayerId, const PlayerInput& remotePlayerInput) {
            if (!mIsSessionRunning) {
                mLogger.logInfoMessage("RollbackManager::OnReceivedRemovePlayerInput", "No running session");
                return;
            }
        }*/

        // FUTURE: Advanced - Manual snapshot support! Eg, for debugging purposes, if a manual snapshot restoration occurs
        //                   outside of rollback manager, then need to restore rollback manager as well!
        //                  Could potentially be used for replay rewinding as well.

        /**
        * Creates a snapshot to restore the internal state of the manager itself.
        * Useful for restoring snapshots outside of rollback process itself, such as during advanced offline replay
        * playback features.
        *
        * Note that this is necessary as restoring "arbitrary" snapshots may result in restoring state outside of the
        * ordinary rollback window.
        * @param result - Resulting snapshot
        **/
        void CreateInternalStateSnapshot(RollbackManagerSnapshot<SnapshotType>& result) {
            result = {}; // Just in case. Should be cheap to do anyways

            result.lastProcessedFrame = mLastProcessedFrame;
            result.inputManager = mInputManager;
            // TODO: This is broken as heck! Well, with FrameSnapshot type at least
            //      2023/03/16: Not sure what above comment is referring to. Perhaps due to pointers in FrameSnapshot
            //      before? But std::vectors should be fine with copying data? Need to carefully test either way.
            result.snapshotManager = mSnapshotManager;
        }

        /**
        * Restores internal state of this manager
        * @param snapshot - Snapshot to restore
        **/
        void RestoreInternalStateSnapshot(const RollbackManagerSnapshot<SnapshotType>& snapshot) {
            mLastProcessedFrame = snapshot.lastProcessedFrame;
            mInputManager = snapshot.inputManager;
            mSnapshotManager = snapshot.snapshotManager;
        }

      private:
        void AssureSettingsAreValid(RollbackSettings& rollbackSettings, RollbackSessionInfo& rollbackSessionInfo) {
            // Assure that input delay is not outside expected range
            auto localInputDelayMagnitude = static_cast<FrameType>(std::abs(rollbackSettings.localInputDelay));
            if (localInputDelayMagnitude > RollbackStaticSettings::kMaxInputDelay) {
                mLogger.logWarnMessage("RollbackManager::OnSessionStart", "Provided input delay is outside expected window");
                rollbackSettings.localInputDelay = 0; // Assure no downstream systems break
            }

            if (rollbackSettings.syncTestFrames > RollbackStaticSettings::kMaxRollbackFrames) {
                rollbackSettings.syncTestFrames = RollbackStaticSettings::kMaxRollbackFrames;
            }
        }
        
        void SetupStateForSessionStart(const RollbackSettings& rollbackSettings, const RollbackSessionInfo& rollbackSessionInfo) {
            // Reset last processed frame.
            //      Note that next frame to process is expected to be 0 as max + 1 = 0
            //      (due to unsigned int overflow being defined behavior) 
            mLastProcessedFrame = std::numeric_limits<FrameType>::max();

            // If local session then try setting up special local-only features
            if (!rollbackSessionInfo.isNetworkedMPSession) {
                // Negative input delay setup (ie, local prediction/forward simulation)
                mIsUsingLocalNegativeInputDelay = rollbackSettings.localInputDelay < 0;
                if (mIsUsingLocalNegativeInputDelay) {
                    mLocalPredictionAmount = static_cast<FrameType>(rollbackSettings.localInputDelay * -1);
                }

                mDoLocalSyncTest = rollbackSettings.useSyncTest && rollbackSettings.syncTestFrames > 0;
                mSyncTestRollbackAmount = rollbackSettings.syncTestFrames;
            }
            else {
                // Disable offline specific features
                mIsUsingLocalNegativeInputDelay = false;
                mDoLocalSyncTest = false;
            }

            mRollbackSessionInfo = rollbackSessionInfo;

            // Setup additional debug settings
            mSyncTestLogSyncTestChecksums = rollbackSettings.logSyncTestChecksums;
            mLogChecksumForEveryStoredFrameSnapshot = rollbackSettings.logChecksumForEveryStoredFrameSnapshot;
        }

        void CheckAndHandleRollbackForNetworkedMPSession() {
            // Nothing to do if not a networked game session
            if (!mRollbackSessionInfo.isNetworkedMPSession) {
                return;
            }
            
            // Check if rollback is appropriate and then handle
            // Also do frame input validation (like OnPostFixedGameplayUpdate)
            mLogger.logWarnMessage(
                "RollbackManager::CheckAndHandleRollbackForNetworkedMPSession",
                "Implement multiplayer session rollback handling plz!"
            );
        }

        void OnFixedGameplayUpdate(const CharacterInput& localPlayerInput) {
            bool didRollbackOccur = false;
            FrameType targetFrame = mLastProcessedFrame + 1;
            
            // If using negative input delay and not on an initial prediction frame...
            if (mIsUsingLocalNegativeInputDelay && targetFrame >= mLocalPredictionAmount) {
                // Need to check if prediction was wrong and thus if need to rollback and re-process those frames
                
                // Get input used for the relevant prediction frame
                const FrameType formerPredictionFrame = targetFrame - mLocalPredictionAmount;
                const CharacterInput predictedInput = mInputManager.GetLocalPlayerPredictedInputForFrame(formerPredictionFrame);

                // Store the relevant input as input manager deems appropriate.
                // Doing this after retrieving prediction as - with current implementation - this will wipe out prediction data. 
                // Also doing before any rollbacks so we have the "source of truth" for relevant frame already stored.
                mInputManager.AddLocalPlayerInput(targetFrame, localPlayerInput);
                
                // Compare against actual input to determine if rollback is necessary for accurate simulation
                bool shouldRollback = predictedInput != localPlayerInput;
                if (shouldRollback) {
                    HandleRollback(formerPredictionFrame);
                    didRollbackOccur = true;
                }
            }
            else {
                // Store input for given frame. This handles both input delay and historic lookup for rollbacks
                mInputManager.AddLocalPlayerInput(targetFrame, localPlayerInput);
            }
            
            // TODO: Send input to remote client (if any)
            
            HandleNextGameplayFrame(false, didRollbackOccur);

            // Handle SyncTest *after* normal processing is done so we can redo the frames again and compare results
            if (mDoLocalSyncTest) {
                HandleSyncTest();
            }

            // If rollback occurred, then we've been calling the non-rendering RollbackUser frame update call. Now that
            // rollback is over, we should let User know that it's time to update rendering
            if (didRollbackOccur) {
                mRollbackUser.OnPostRollback();
            }
        }

        void HandleSyncTest() {
            // Redundant sanity check
            if (mSyncTestRollbackAmount == 0) {
                mLogger.logWarnMessage("RollbackManager::HandleSyncTest", "Rollback frame amount is 0!");
                return;
            }
            // If in initial frames - so can't rollback far enough - then don't bother testing
            if (mLastProcessedFrame <  mSyncTestRollbackAmount) {
                return;
            }
            
            // Grab current snapshot's checksum so we know what to compare against
            uint32_t preTestSnapshotChecksum = mSnapshotManager.GetSnapshot(mLastProcessedFrame).CalculateChecksum();
            
            // Do normal rollback process
            // Note that OnFixedGameplayUpdate() doesn't care that we call HandleRollback here as we expect no different
            // state and thus no difference in visuals
            FrameType firstFrameToReprocess = mLastProcessedFrame - mSyncTestRollbackAmount;
            HandleRollback(firstFrameToReprocess);

            // Finally compare hashes and output result
            uint32_t postTestSnapshotChecksum = mSnapshotManager.GetSnapshot(mLastProcessedFrame).CalculateChecksum();
            if (preTestSnapshotChecksum != postTestSnapshotChecksum) {
                mLogger.logWarnMessage(
                    "RollbackManager::HandleSyncTest",
                    "SyncTest failed for frame " + std::to_string(mLastProcessedFrame)
                );
            }

            // Optionally log additional info to help with arbitrary debugging (as ended up adding this log statement multiple times)
            if (mSyncTestLogSyncTestChecksums) {
                mLogger.logInfoMessage(
                    "RollbackManager::HandleSyncTest",
                    "Checksum Pre: " + std::to_string(preTestSnapshotChecksum) +
                    " | Post: " + std::to_string(postTestSnapshotChecksum)
                );
            }
        }

        /**
        * Actually do gameplay update for next frame
        * @param skipSnapshotCreation - true if should skip snapshot creation, such as when re-processing the first
        *                               frame of a series since the snapshot would be identical to what's stored
        * @param didRollbackOccur - true if rollback already occurred in this manager update
        **/
        void HandleNextGameplayFrame(bool skipSnapshotCreation, bool didRollbackOccur) {
            FrameType targetFrame = mLastProcessedFrame + 1; // Don't take this as an input to assure we always do the "next" frame

            if (!skipSnapshotCreation) {
                // Store game state at START of frame.
                // This works around how to re-process first (0th) frame and thus how to rollback to before then
                StoreSnapshot(targetFrame);
            }
            
            // Grab input(s) for updating game
            const CharacterInput& localPlayerInput = mInputManager.GetLocalPlayerInput(targetFrame);

            // Update game. Note that this is also expected to increment RollbackUser's frame tracking as well
            if (!didRollbackOccur) {
                mRollbackUser.ProcessFrame(targetFrame, localPlayerInput);
            }
            else {
                mRollbackUser.ProcessFrameWithoutRendering(targetFrame, localPlayerInput);
            }

            // Finally internally remember that we processed this frame
            mLastProcessedFrame = targetFrame;
        }

        /**
        * Handles process of rolling back then re-processing relevant frames. Relevant inputs are expected to be stored
        * before calling this method.
        * @param firstFrameToReprocess - what frame was incorrect and thus needs to start re-processing from
        **/
        void HandleRollback(FrameType firstFrameToReprocess) {
            // Calculate number of frames we actually need to re-process
            // Eg: Last processed frame = 2, input = 0, so we want to redo 3 frames (0 through 2)
            const FrameType numOfFramesToProcess = mLastProcessedFrame - firstFrameToReprocess + 1;

            // Sanity check input
            if (firstFrameToReprocess > mLastProcessedFrame) { // Should never occur during normal play (incl overflow)
                mLogger.logErrorMessage(
                    "RollbackManager::HandleRollback",
                    "Non-existent frame to re-process! Last processed frame: " + std::to_string(mLastProcessedFrame) +
                    ", input frame: " + std::to_string(firstFrameToReprocess)
                );
                return;
            }
            if (numOfFramesToProcess > RollbackStaticSettings::kMaxRollbackFrames) {
                mLogger.logErrorMessage(
                    "RollbackManager::HandleRollback",
                    "Trying to rollback beyond supported window! Last processed frame: " +
                    std::to_string(mLastProcessedFrame) + ", input frame: " + std::to_string(firstFrameToReprocess)
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
                HandleNextGameplayFrame(skipSnapshotCreation, true);
            }
        }

        /**
        * Generates and stores a snapshot so we can re-process the target frame if/when necessary
        * @param targetFrame - intended frame to generate snapshot for. This should be BEFORE frame is processed.
        **/
        void StoreSnapshot(FrameType targetFrame) {
            // Likely redundant sanity checks BUT could uncover bugs in future!
            if (IsFrameValueMax(targetFrame)) {
                mLogger.logErrorMessage(
                    "RollbackManager::StoreSnapshot",
                    "Invalid frame (max value) to store snapshot for!: Input frame: " + std::to_string(targetFrame)
                );
                return;
            }
            if (targetFrame > mLastProcessedFrame + 1) {
                mLogger.logErrorMessage(
                    "RollbackManager::StoreSnapshot",
                    "Trying to store snapshot for unprocessed frame! Last processed frame: " +
                    std::to_string(mLastProcessedFrame) + ", input frame: " + std::to_string(targetFrame)
                );
                return;
            }

            {   // Create then swap-replace insert snapshot.
                //      Using scope delimiters to make explicit that variable should NOT be used after this cuz of the
                //      store call using swap-replace.
                SnapshotType snapshot = {}; 
                mRollbackUser.GenerateSnapshot(targetFrame, snapshot);
                mSnapshotManager.StoreSnapshot(targetFrame, snapshot); 
            }

            if (mLogChecksumForEveryStoredFrameSnapshot) {
                uint32_t curSnapshotChecksum = mSnapshotManager.GetSnapshot(targetFrame).CalculateChecksum();
                mLogger.logInfoMessage(
                    "RollbackManager::StoreSnapshot",
                    "Frame " + std::to_string(targetFrame) + ": " + std::to_string(curSnapshotChecksum)
                );
            }
        }

        void RestoreSnapshot(FrameType frameToReprocess) {
            // Likely redundant sanity checks BUT could uncover bugs in future!
            if (IsFrameValueMax(frameToReprocess)) {
                mLogger.logErrorMessage(
                    "RollbackManager::RestoreSnapshot",
                    "Invalid frame (max value) to retrieve snapshot for!: Input frame: " + std::to_string(frameToReprocess)
                );
                return;
            }
            if (frameToReprocess > mLastProcessedFrame) { // Snapshots are expected to be generated BEFORE an update occurs
                mLogger.logErrorMessage(
                    "RollbackManager::RestoreSnapshot",
                    "Trying to retrieve snapshot for unprocessed frame! Last processed frame: " +
                    std::to_string(mLastProcessedFrame) + ", input frame: " + std::to_string(frameToReprocess)
                );
                return;
            }
            
            // Get and restore game snapshot
            const SnapshotType& snapshot = mSnapshotManager.GetSnapshot(frameToReprocess);
            mRollbackUser.RestoreSnapshot(frameToReprocess, snapshot);

            // Also update necessary internal state, which is just this manager's frame tracking at the moment.
            //  Yes, at time of writing ALL other rollback state is either history based (so shouldn't be overwritten)
            //  or session based (so no need to restore)
            mLastProcessedFrame = frameToReprocess - 1;
        }

        void CheckAndNotifyConfirmedFrameCount(bool didAnyLocalFixedGameplayFramesOccur) {
            // Sanity check
            if (didAnyLocalFixedGameplayFramesOccur && IsFrameValueMax(mLastProcessedFrame)) {
                mLogger.logWarnMessage(
                    "RollbackManager::CheckAndNotifyConfirmedFrameCount",
                    "Current frame value is still at max after gameplay update supposedly occurred!"
                );
                return;
            }
            
            // FUTURE: Handle proper input validation with input packets coming from other players (including if other people are ahead)
            if (mRollbackSessionInfo.isNetworkedMPSession) {
                mLogger.logWarnMessage(
                    "RollbackManager::OnPostUpdate",
                    "Input confirmation not yet implemented for multiplayer?!"
                );
                
                return; // Rest of code below assumes local-only session
            }

            // For now cuz no multiplayer, if no fixed gameplay frames happened then we did no relevant work.
            //      CAUTION: Runahead case is a bit tricky as may occur without fixed gameplay update.
            //      Should still ultimately be fine for now.... methinks? Needs (unit) testing to make sure always valid assumption!
            if (!didAnyLocalFixedGameplayFramesOccur) {
                return;
            }

            FrameType curLocalRollbackRange = GetMaxLocalRollbackFrames();
            if (curLocalRollbackRange > 0) { // Using local rollback at all?
                if (mLastProcessedFrame >= curLocalRollbackRange) { // If beyond "initial" frames, then have frames we can confirm
                    // For simplified logic, just validate any frame once it exits the possible rollback range.
                    // (Technically we could consider inputs never changing for synctest and whatnot, but no need to complicate further)
                    mRollbackUser.OnInputsExitRollbackWindow(mLastProcessedFrame - curLocalRollbackRange);
                }
            }
            // Otherwise not using any form of rollback at all in the current session, so immediately validate every frame
            else {
                mRollbackUser.OnInputsExitRollbackWindow(mLastProcessedFrame);
            }
        }

        FrameType GetMaxLocalRollbackFrames() const {
            FrameType result = 0;
            
            if (mDoLocalSyncTest) {
                result += mSyncTestRollbackAmount;
            }
            if (mIsUsingLocalNegativeInputDelay) {
                result += mLocalPredictionAmount;
            }

            return result;
        }

        /**
        * Checks if the given frame is at the max possible value.
        * The underlying intention here is to easily identify what code is relying on this assumption (mostly for sanity checks).
        * We'll need to review these use cases in future to see if super long games with frame count overflow are feasible.
        * (For now, don't think are feasible cuz 4,294,967,295 - current FrameType max value - is hundreds of days long with 60fps!)
        * @param frame - Frame value to check
        * @returns true if frame value is at the max possible value for the numeric type
        **/
        bool IsFrameValueMax(FrameType frame) const {
            return frame == std::numeric_limits<FrameType>::max();
        }
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();

        RollbackUser<SnapshotType>& mRollbackUser;
        RollbackSessionInfo mRollbackSessionInfo;

        #pragma region Session Settings
        bool mDoLocalSyncTest = false;
        FrameType mSyncTestRollbackAmount = 0;
        
        bool mIsUsingLocalNegativeInputDelay = false;
        FrameType mLocalPredictionAmount = 0;

        // Dedicated arbitrary debug settings
        bool mSyncTestLogSyncTestChecksums = false;
        bool mLogChecksumForEveryStoredFrameSnapshot = false;
        #pragma endregion
        
        RollbackInputManager mInputManager = {};
        RollbackSnapshotManager<SnapshotType>& mSnapshotManager;

        bool mIsSessionRunning = false;
        // Should always be one less than next frame to process (including overflow) 
        FrameType mLastProcessedFrame = std::numeric_limits<FrameType>::max();
    };
}
