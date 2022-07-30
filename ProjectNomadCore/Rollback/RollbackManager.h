#pragma once

#include "InputPredictor.h"
#include "GameCore/PlayerId.h"
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
        void OnSessionStart(const RollbackSettings& rollbackSettings, const RollbackSessionInfo& rollbackSessionInfo) {
            if (mIsSessionRunning) {
                mLogger.logWarnMessage("RollbackManager::StartGame", "Start called while already running!");
            }
            
            SetupStateForSessionStart(rollbackSettings, rollbackSessionInfo);
            mIsSessionRunning = true;
        }
        
        void EndSession() {
            // Simply mark session as stopped running.
            // No need to clear existing data as all other methods check for this explicitly
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

            CheckAndHandleRollbackForNetworkedMPSession();
            
            // Any further work is only appropriate for fixed gameplay frames
            if (isFixedGameplayFrame) {
                OnFixedGameplayUpdate(currentFrame, localPlayerInput);
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
        void SetupStateForSessionStart(const RollbackSettings& rollbackSettings, const RollbackSessionInfo& rollbackSessionInfo) {
            // Reset relevant state
            mLastProcessedFrame = 0;

            // Setup relevant settings
            mRollbackSettings = rollbackSettings;
            mRollbackSessionInfo = rollbackSessionInfo;
        }

        void CheckAndHandleRollbackForNetworkedMPSession() {
            // Nothing to do if not a networked game session
            if (!mRollbackSessionInfo.isNetworkedMPSession) {
                return;
            }
            
            // TODO: Check if rollback is appropriate and then handle
        }

        void OnFixedGameplayUpdate(FrameType currentFrame, const PlayerInput& localPlayerInput) {
            // Debug helper: Check that frame is expected, which should always be called one frame forward
            // (aside from first frame processing)
            if (mLastProcessedFrame != 0 && currentFrame != mLastProcessedFrame + 1) {
                mLogger.logWarnMessage(
                    "RollbackManager::OnFixedGameplayUpdate",
                    "Received unexpected frame! mLastProcessedFrame: " + std::to_string(mLastProcessedFrame) +
                    ", currentFrame: " + std::to_string(currentFrame)
                );
            }
            
            // Store input for given frame
            currentFrameLocalInput = localPlayerInput;
            // TODO: Send input to remote client (if any)
            // TODO: If local and doing negative input delay, then check against prediction and potentially rollback

            // Finally handle gameplay update for current frame
            HandleGameplayFrame(currentFrame);
            mLastProcessedFrame = currentFrame;
        }

        void HandleGameplayFrame(FrameType targetFrame) {
            // Grab input(s) for updating game
            const PlayerInput& localPlayerInput = currentFrameLocalInput;

            // Update game
            mRollbackUser.ProcessFrame(localPlayerInput);
            
            // Store game state
            SnapshotType snapshot = {};
            mRollbackUser.GenerateSnapshot(snapshot);
            mSnapshotManager.storeSnapshot(targetFrame, snapshot);
        }
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();

        RollbackUser<SnapshotType>& mRollbackUser;
        RollbackSettings mRollbackSettings;
        RollbackSessionInfo mRollbackSessionInfo;
        
        RollbackSnapshotManager<SnapshotType> mSnapshotManager;
        InputPredictor mInputPredictor;
        // TODO: RollbackInputManager
        PlayerInput currentFrameLocalInput;

        bool mIsSessionRunning = false;
        FrameType mLastProcessedFrame = 0;
    };
}
