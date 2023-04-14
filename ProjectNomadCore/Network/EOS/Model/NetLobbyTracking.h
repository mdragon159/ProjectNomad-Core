#pragma once
#include "Model/EOSLobbyProperties.h"

namespace ProjectNomad {
    // NOTE: If any status enum is added, then review consumers of all code consuming each state (eg, delayed join logic)
    enum class NetLobbyTrackingStatus : uint8_t {
        Inactive,
        BeingCreated,       // Waiting for lobby to be created
        JoinInProgress,   // Waiting for lobby to be joined
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
        bool IsJoinInProgress() const {
            return mTrackingStatus == NetLobbyTrackingStatus::JoinInProgress;
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

        bool HasActiveDelayedJoinRequest() const {
            return mDelayedJoinRequestLobby != nullptr;
        }
        EOSLobbyDetailsKeeper GetAndClearDelayedJoinRequest() {
            EOSLobbyDetailsKeeper result = mDelayedJoinRequestLobby;
            mDelayedJoinRequestLobby.reset();
            return result;
        }

        const EOSLobbyProperties& GetLobbyProperties() const {
            return mLobbyProperties;
        }

        

        void SetBeingCreatedState(LoggerSingleton& logger) {
            // Sanity check
            if (!IsCompletelyInactive()) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::SetBeingCreatedState",
                    "Unexpected state atm! Current state: " + std::to_string(GetCurrentStatusAsNumber())
                );
                return; // Don't mask issue so easier to identify
            }
            
            mTrackingStatus = NetLobbyTrackingStatus::BeingCreated;
        }
        void SetJoinInProgressState(LoggerSingleton& logger) {
            // Sanity check
            if (!IsCompletelyInactive()) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::SetJoinInProgressState",
                    "Unexpected state atm! Current state: " + std::to_string(GetCurrentStatusAsNumber())
                );
                return; // Don't mask issue so easier to identify
            }
            
            mTrackingStatus = NetLobbyTrackingStatus::JoinInProgress;
        }
        void SetLeaveInProgressState(LoggerSingleton& logger) {
            // Sanity check
            if (!IsActive()) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::SetLeaveInProgress",
                    "Unexpected that not in active state atm! Current state: " + std::to_string(GetCurrentStatusAsNumber())
                );
                return; // Don't mask issue so easier to identify
            }
            
            mTrackingStatus = NetLobbyTrackingStatus::LeaveInProgress;

            // Note that we should not be clearing the lobby properties tracking yet as that data may still be read.
            //      (Eg, by the EOS SDK itself)
        }
        
        bool TryInitActiveLobby(LoggerSingleton& logger,
                                const CrossPlatformIdWrapper& localPlayerId,
                                EOS_HLobby lobbyHandle,
                                EOS_LobbyId inputId) {
            // Sanity check
            if (!IsBeingCreated() && !IsJoinInProgress()) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::InitActiveLobby",
                    "Unexpected existing state. Sign of incorrect lobby setup! Current status: "
                    + std::to_string(GetCurrentStatusAsNumber())
                );
                return false;
            }

            // Try to copy the actual lobby info
            bool didLobbyPropsInit = mLobbyProperties.TryInit(logger, localPlayerId, lobbyHandle, inputId);
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

        bool TryUpdateActiveLobby(LoggerSingleton& logger,
                                  const CrossPlatformIdWrapper& localPlayerId,
                                  EOS_HLobby lobbyHandle,
                                  EOS_LobbyId inputId) {
            // Sanity check
            if (!IsActive()) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::TryUpdateActiveLobby",
                    "Lobby not already in active state. Sign of incorrect lobby setup! Current status: "
                    + std::to_string(GetCurrentStatusAsNumber())
                );
                return false;
            }

            mLobbyProperties = {}; // Not really necessary but nice to be explicit that throwing away all data atm

            bool didLobbyPropsInit = mLobbyProperties.TryInit(logger, localPlayerId, lobbyHandle, inputId);
            if (!didLobbyPropsInit) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::TryUpdateActiveLobby",
                    "Failed to init lobby properties. See prior logs (as should have logged earlier)"
                );
                return false;
            }

            return true;
        }

        // Caller could just use re-initialization, but felt safer to define this when initially writing up class
        //      May delete later if unnecessary/not helpful.
        void ResetLobbyInfo(bool clearAllTrackingInfo) {
            mTrackingStatus = NetLobbyTrackingStatus::Inactive;
            mLobbyProperties = {};

            if (clearAllTrackingInfo) {
                // Reset all other tracking info (perhaps such as when shutting down netcode or when netcode reached erroneous state)
                mDelayedJoinRequestLobby.reset();
            }
        }

        

        void SetActiveDelayedJoinRequest(LoggerSingleton& logger, const EOSLobbyDetailsKeeper& targetLobbyHandle) {
            if (IsCompletelyInactive()) {
                logger.addWarnNetLog(
                    "NetLobbyTracking::SetActiveJoinRequest",
                    "Lobby tracking is currently in inactive state, should not be doing a delayed join!"
                );
                return; // Tracking and logic overall is likely already in a broken state, so don't mask issues by immediately stopping
            }

            // Note that we *do not* care about overwriting a previous join request, as then simply changing upcoming
            //      join request target.
            //      ie, we always reset this value when actually trying to use it for a join. So no downstream effect
            //      for re-setting the value.
            
            mDelayedJoinRequestLobby = targetLobbyHandle;
        }

      private:
        NetLobbyTrackingStatus mTrackingStatus = NetLobbyTrackingStatus::Inactive;

        EOSLobbyDetailsKeeper mDelayedJoinRequestLobby = nullptr;
        EOSLobbyProperties mLobbyProperties = {};
    };
}
