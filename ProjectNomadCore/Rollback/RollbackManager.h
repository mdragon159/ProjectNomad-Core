#pragma once

#include "RollbackManagerMode.h"
#include "Interface/RollbackUser.h"
#include "Model/RollbackSettings.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    /**
    * Ingress point for all rollback behavior.
    * ie, this class encapsulates all necessary logic for rollback-related features
    * @tparam SnapshotType - defines struct used for frame snapshot. "Restoring" this should effectively return to a prior frame
    **/
    template <typename SnapshotType>
    class RollbackManager {
      public:
        RollbackManager() = default;

        void StartGame(RollbackSettings rollbackSettings) {
            if (mCurrentMode == RollbackManagerMode::Running) {
                mLogger.logWarnMessage("RollbackManager::StartGame", "Start called while already running!");
            }

            // TODO: Starting stuff
            ResetState();

            mCurrentMode = RollbackManagerMode::Running;
        }

        void OnFixedUpdate() {
            //
        }

        void OnReplayDrivenFixedUpdate() {
            //
        }

        void OnReceivedRemovePlayerInput() {
            // TODO
        }

      private:
        void ResetState() {
            //
        }
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();

        RollbackUser<SnapshotType>& mRollbackUser;

        RollbackManagerMode mCurrentMode = RollbackManagerMode::NotStarted;
    };
}
