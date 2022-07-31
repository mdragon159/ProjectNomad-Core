#pragma once

#include "RollbackInputManager.h"
#include "Interface/RollbackUser.h"
#include "Model/RollbackSessionInfo.h"
#include "Model/RollbackSettings.h"
#include "Model/RollbackSnapshotManager.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    /**
    * Ingress point for all rollback behavior.
    * ie, this class encapsulates all necessary logic for rollback-related features
    *
    * Also, useful high level general rollback logic outline: https://gist.github.com/rcmagic/f8d76bca32b5609e85ab156db38387e9
    * @tparam SnapshotType - defines struct used for frame snapshot. "Restoring" this should effectively return to a prior frame
    **/
    template <typename SnapshotType>
    class RollbackManager {
      public:
        RollbackManager(RollbackUser<SnapshotType>& rollbackUser) : mRollbackUser(rollbackUser) {}

        /**
        * Expected to be called at start of new game session before any other method is called.
        * @param rollbackSettings - settings for rollback behavior such as input delay
        * @param rollbackSessionInfo - session-specific settings such as number of players
        **/
        void OnSessionStart(RollbackSettings rollbackSettings, RollbackSessionInfo rollbackSessionInfo) {
            if (mIsSessionRunning) {
                mLogger.logWarnMessage("RollbackManager::OnSessionStart", "Start called while already running!");
            }

            AssureSettingsAreValid(rollbackSettings, rollbackSessionInfo);
            
            SetupStateForSessionStart(rollbackSettings, rollbackSessionInfo);
            mInputManager.OnSessionStart(rollbackSettings, rollbackSessionInfo);
            
            mIsSessionRunning = true;
        }
        
        void EndSession() {
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
        void OnUpdate(bool isFixedGameplayFrame, FrameType currentFrame, const PlayerInput& localPlayerInput) {
            if (!mIsSessionRunning) {
                mLogger.logInfoMessage("RollbackManager::OnFixedUpdate", "No running session");
                return;
            }
            // Sanity check: Verify that the frame is expected, which should always be called one frame forward
            // (aside from first frame processing)
            if (currentFrame != mLastProcessedFrame + 1) {
                mLogger.logErrorMessage(
                    "RollbackManager::OnUpdate",
                    "Received unexpected frame which should be one greater than last processed frame! mLastProcessedFrame: "
                    + std::to_string(mLastProcessedFrame) + ", provided frame: " + std::to_string(currentFrame)
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

      private:
        void AssureSettingsAreValid(RollbackSettings& rollbackSettings, RollbackSessionInfo& rollbackSessionInfo) {
            // Assure that input delay is not outside expected range
            auto localInputDelayMagnitude = static_cast<FrameType>(std::abs(rollbackSettings.localInputDelay));
            if (localInputDelayMagnitude > RollbackStaticSettings::kMaxInputDelay) {
                mLogger.logWarnMessage("RollbackManager::OnSessionStart", "Provided input delay is outside expected window");
                rollbackSettings.localInputDelay = 0; // Assure no downstream systems break
            }
        }
        
        void SetupStateForSessionStart(const RollbackSettings& rollbackSettings, const RollbackSessionInfo& rollbackSessionInfo) {
            // Reset last processed frame. Note that next frame to process is expected to be 0 as max + 1 = 0
            // (due to unsigned int overflow being defined behavior) 
            mLastProcessedFrame = std::numeric_limits<FrameType>::max();

            // If local session then try setting up special local-only features
            if (!rollbackSessionInfo.isNetworkedMPSession) {
                // Negative input delay setup (ie, local prediction/forward simulation)
                mIsUsingLocalNegativeInputDelay = rollbackSettings.onlineInputDelay < 0;
                if (mIsUsingLocalNegativeInputDelay) {
                    mLocalPredictionAmount = static_cast<FrameType>(rollbackSettings.localInputDelay * -1);
                }
            }

            mRollbackSessionInfo = rollbackSessionInfo;
        }

        void CheckAndHandleRollbackForNetworkedMPSession() {
            // Nothing to do if not a networked game session
            if (!mRollbackSessionInfo.isNetworkedMPSession) {
                return;
            }
            
            // Check if rollback is appropriate and then handle
        }

        void OnFixedGameplayUpdate(const PlayerInput& localPlayerInput) {
            bool didRollbackOccur = false;
            FrameType targetFrame = mLastProcessedFrame + 1;
            
            // If using negative input delay and not on an initial prediction frame...
            if (mIsUsingLocalNegativeInputDelay && targetFrame >= mLocalPredictionAmount) {
                // Need to check if prediction was wrong and thus if need to rollback and re-process those frames
                
                // Get input used for the relevant prediction frame
                const FrameType formerPredictionFrame = targetFrame - mLocalPredictionAmount;
                const PlayerInput predictedInput = mInputManager.GetLocalPlayerPredictedInputForFrame(formerPredictionFrame);

                // Store the relevant input as input manager deems appropriate.
                // Doing this after retrieving prediction as - with current implementation - this will wipe out prediction data. 
                // Also doing before any rollbacks so we have the "source of truth" for relevant frame already stored.
                mInputManager.AddLocalPlayerInput(targetFrame, localPlayerInput);
                
                // Compare against actual input (TODO: Is same frame input?)
                bool shouldRollback = predictedInput != localPlayerInput;
                if (shouldRollback) {
                    mLogger.logInfoMessage("OnFixedGameplayUpdate", "Rollback situation detected yay!");
                    
                    HandleRollback(formerPredictionFrame);
                    didRollbackOccur = true;
                }
            }
            else {
                // Store input for given frame. This both handles both input delay and historic lookup for rollbacks
                mInputManager.AddLocalPlayerInput(targetFrame, localPlayerInput);
            }
            
            // TODO: Send input to remote client (if any)

            HandleGameplayFrame(didRollbackOccur);
            if (didRollbackOccur) {
                mRollbackUser.OnPostRollback();
            }
        }

        /**
        * Actually do gameplay update for next frame
        **/
        void HandleGameplayFrame(bool didRollbackOccur) {
            FrameType targetFrame = mLastProcessedFrame + 1;
            
            // Grab input(s) for updating game
            const PlayerInput& localPlayerInput = mInputManager.GetLocalPlayerInput(targetFrame);

            // Update game. Note that this is also expected to increment RollbackUser's frame tracking as well
            if (!didRollbackOccur) {
                mRollbackUser.ProcessFrame(targetFrame, localPlayerInput);
            }
            else {
                mRollbackUser.ProcessFrameWithoutRendering(targetFrame, localPlayerInput);
            }
            
            // Store game state
            SnapshotType snapshot = {};
            mRollbackUser.GenerateSnapshot(snapshot);
            mSnapshotManager.storeSnapshot(targetFrame, snapshot);
            // TODO: Store internal rollback manager state

            // Finally internally remember that we processed this frame
            mLastProcessedFrame = targetFrame;
        }

        /**
        * Handles process of rolling back then re-processing relevant frames
        * @param firstFrameToReprocess - what frame was incorrect and thus needs to start re-processing from
        **/
        void HandleRollback(FrameType firstFrameToReprocess) {
            // TODO: Sanity check inputs

            // Eg: mLastProcessedFrame = 2, input = 0, want to redo frames 4 frames (0 through 2)
            const FrameType numOfFramesToProcess = mLastProcessedFrame - firstFrameToReprocess;
            
            // TODO: Implement rollback!
            // 1. Restore snapshot for target frame (both for User and for self)
            // 2. Process frames starting at frameToRollbackTo and up to mLocalPredictionAmount - 1
            //    1. Simply call HandleGameplayFrame with rollback bool set
        }
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();

        RollbackUser<SnapshotType>& mRollbackUser;
        RollbackSessionInfo mRollbackSessionInfo;

        bool mIsUsingLocalNegativeInputDelay = false;
        FrameType mLocalPredictionAmount = 0;
        
        RollbackInputManager mInputManager;
        // InputPredictor mInputPredictor;
        RollbackSnapshotManager<SnapshotType> mSnapshotManager;

        bool mIsSessionRunning = false;
        // Should always be one less than next frame to process (including overflow) 
        FrameType mLastProcessedFrame = std::numeric_limits<FrameType>::max();
    };
}
