#pragma once
#include "EOSLobbyProperties.h"

namespace ProjectNomad {
    enum class NetLobbyTrackingStatus : uint8_t {
        Inactive,
        BeingCreated,       // Waiting for lobby to be created
        Active,             // Created and joined
        LeaveInProgress     // Trying to leave lobby. (Not sure why destroy is instant in SDK samples but eh, best to be safe here)
    };
    
    class NetLobbyTracking {
      public:
        bool IsCompletelyInactive() const {
            return mTrackingStatus == NetLobbyTrackingStatus::Inactive;
        }
        bool IsBeingCreated() const {
            return mTrackingStatus == NetLobbyTrackingStatus::BeingCreated;
        }
        bool IsActive() const {
            return mTrackingStatus == NetLobbyTrackingStatus::Active;
        }
        bool IsLeaveInProgress() const {
            return mTrackingStatus == NetLobbyTrackingStatus::LeaveInProgress;
        }
        int GetCurrentStatusAsNumber() const {
            return static_cast<int>(mTrackingStatus);
        }

        void SetBeingCreatedState(LoggerSingleton& logger) {
            // Sanity check
            if (mTrackingStatus != NetLobbyTrackingStatus::Inactive) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::SetBeingCreatedState",
                    "Unexpected state atm! Current state: " + GetCurrentStatusAsNumber()
                );
                return; // Don't mask issue so easier to identify
            }
            
            mTrackingStatus = NetLobbyTrackingStatus::BeingCreated;
        }
        void SetLeaveInProgressState(LoggerSingleton& logger) {
            // Sanity check
            if (mTrackingStatus != NetLobbyTrackingStatus::Active) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::SetLeaveInProgress",
                    "Unexpected that not in active state atm! Current state: " + GetCurrentStatusAsNumber()
                );
                return; // Don't mask issue so easier to identify
            }
            
            mTrackingStatus = NetLobbyTrackingStatus::LeaveInProgress;

            // Note that we should not be clearing the lobby properties tracking yet as that data may still be read.
            //      (Eg, by the EOS SDK itself)
        }
        
        bool TryInitActiveLobby(LoggerSingleton& logger, EOS_LobbyId inputId) {
            // Sanity check
            if (mTrackingStatus != NetLobbyTrackingStatus::BeingCreated) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::InitActiveLobby",
                    "Unexpected existing state. Sign of incorrect lobby setup! Current status: "
                    + GetCurrentStatusAsNumber()
                );
                return false;
            }

            // Try to copy the actual lobby info
            bool didLobbyPropsInit = mLobbyProperties.TryInit(logger, inputId);
            if (!didLobbyPropsInit) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::InitActiveLobby",
                    "Failed to init lobby properties. See prior logs (as should have logged earlier)"
                );
                return false;
            }

            mTrackingStatus = NetLobbyTrackingStatus::Active;
            return true; // Initializing active lobby info succeeded yay!
        }

        // Caller could just use re-initialization, but felt safer to define this when initially writing up class
        //      May delete later if unnecessary/not helpful.
        void ResetLobbyInfo() {
            mTrackingStatus = NetLobbyTrackingStatus::Inactive;
            mLobbyProperties = {};
        }

        const EOSLobbyProperties& GetLobbyProperties() const {
            return mLobbyProperties;
        }

      private:
        NetLobbyTrackingStatus mTrackingStatus = NetLobbyTrackingStatus::Inactive;

        EOSLobbyProperties mLobbyProperties = {};
    };
}
