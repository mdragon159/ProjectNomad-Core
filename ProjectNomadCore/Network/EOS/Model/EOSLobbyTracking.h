#pragma once
#include "EOSPlayersInfoTracking.h"
#include "Model/EOSLobbyProperties.h"

namespace ProjectNomad {
    // NOTE: If any status enum is added, then review consumers of all code consuming each state (eg, delayed join logic)
    enum class EOSLobbyTrackingStatus : uint8_t {
        Inactive,
        BeingCreated,       // Waiting for lobby to be created
        JoinInProgress,   // Waiting for lobby to be joined
        Active,             // Created and joined
        LeaveInProgress     // Trying to leave lobby. (Not sure why destroy is instant in SDK samples but eh, best to be safe here)
    };
    
    class EOSLobbyTracking {
      public:
        bool IsCompletelyInactive() const {
            return mTrackingStatus == EOSLobbyTrackingStatus::Inactive;
        }
        bool IsBeingCreated() const {
            return mTrackingStatus == EOSLobbyTrackingStatus::BeingCreated;
        }
        bool IsJoinInProgress() const {
            return mTrackingStatus == EOSLobbyTrackingStatus::JoinInProgress;
        }
        bool IsActive() const {
            return mTrackingStatus == EOSLobbyTrackingStatus::Active;
        }
        bool IsLeaveInProgress() const {
            return mTrackingStatus == EOSLobbyTrackingStatus::LeaveInProgress;
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

        

        void SetBeingCreatedState() {
            // Sanity check
            if (!IsCompletelyInactive()) {
                mLogger.AddWarnNetLog(
                    "NetLobbyTracking::SetBeingCreatedState",
                    "Unexpected state atm! Current state: " + std::to_string(GetCurrentStatusAsNumber())
                );
                return; // Don't mask issue so easier to identify
            }
            
            mTrackingStatus = EOSLobbyTrackingStatus::BeingCreated;
        }
        void SetJoinInProgressState() {
            // Sanity check
            if (!IsCompletelyInactive()) {
                mLogger.AddWarnNetLog(
                    "NetLobbyTracking::SetJoinInProgressState",
                    "Unexpected state atm! Current state: " + std::to_string(GetCurrentStatusAsNumber())
                );
                return; // Don't mask issue so easier to identify
            }
            
            mTrackingStatus = EOSLobbyTrackingStatus::JoinInProgress;
        }
        void SetLeaveInProgressState() {
            // Sanity check
            if (!IsActive()) {
                mLogger.AddWarnNetLog(
                    "NetLobbyTracking::SetLeaveInProgress",
                    "Unexpected that not in active state atm! Current state: " + std::to_string(GetCurrentStatusAsNumber())
                );
                return; // Don't mask issue so easier to identify
            }
            
            mTrackingStatus = EOSLobbyTrackingStatus::LeaveInProgress;

            // Note that we should not be clearing the lobby properties tracking yet as that data may still be read.
            //      (Eg, by the EOS SDK itself)
        }
        
        bool TryInitActiveLobby(const CrossPlatformIdWrapper& localPlayerId,
                                EOS_HLobby lobbyHandle,
                                EOS_LobbyId inputId,
                                EOSPlayersInfoTracking& netUsersInfoTracking) {
            // Sanity check
            if (!IsBeingCreated() && !IsJoinInProgress()) {
                mLogger.AddWarnNetLog(
                    "NetLobbyTracking::InitActiveLobby",
                    "Unexpected existing state. Sign of incorrect lobby setup! Current status: "
                    + std::to_string(GetCurrentStatusAsNumber())
                );
                return false;
            }

            // Try to copy the actual lobby info
            bool didLobbyPropsInit = mLobbyProperties.TryInit(mLogger, localPlayerId, lobbyHandle, inputId);
            if (!didLobbyPropsInit) {
                mLogger.AddWarnNetLog(
                    "NetLobbyTracking::InitActiveLobby",
                    "Failed to init lobby properties. See prior logs (as should have logged earlier)"
                );
                return false;
            }

            // Request any missing info for lobby members that netcode "user" side will need (like display names)
            AddUserInfoQueriesForLobbyMembers(netUsersInfoTracking);

            mTrackingStatus = EOSLobbyTrackingStatus::Active;
            return true; // Initializing active lobby info succeeded yay!
        }

        bool TryUpdateActiveLobby(const CrossPlatformIdWrapper& localPlayerId,
                                  EOS_HLobby lobbyHandle,
                                  EOS_LobbyId inputId,
                                  EOSPlayersInfoTracking& netUsersInfoTracking) {
            // Sanity check
            if (!IsActive()) {
                mLogger.AddWarnNetLog(
                    "NetLobbyTracking::TryUpdateActiveLobby",
                    "Lobby not already in active state. Sign of incorrect lobby setup! Current status: "
                    + std::to_string(GetCurrentStatusAsNumber())
                );
                return false;
            }

            mLobbyProperties = {}; // Not really necessary but nice to be explicit that throwing away all data atm

            bool didLobbyPropsInit = mLobbyProperties.TryInit(mLogger, localPlayerId, lobbyHandle, inputId);
            if (!didLobbyPropsInit) {
                mLogger.AddWarnNetLog(
                    "NetLobbyTracking::TryUpdateActiveLobby",
                    "Failed to init lobby properties. See prior logs (as should have logged earlier)"
                );
                return false;
            }

            // Request any missing info for lobby members that netcode "user" side will need (like display names)
            AddUserInfoQueriesForLobbyMembers(netUsersInfoTracking);

            return true;
        }

        // Caller could just use re-initialization, but felt safer to define this when initially writing up class
        //      May delete later if unnecessary/not helpful.
        void ResetLobbyInfo(bool clearAllTrackingInfo) {
            mTrackingStatus = EOSLobbyTrackingStatus::Inactive;
            mLobbyProperties = {};

            if (clearAllTrackingInfo) {
                // Reset all other tracking info (perhaps such as when shutting down netcode or when netcode reached erroneous state)
                mDelayedJoinRequestLobby.reset();
            }
        }

        

        void SetActiveDelayedJoinRequest(const EOSLobbyDetailsKeeper& targetLobbyHandle) {
            if (IsCompletelyInactive()) {
                mLogger.AddWarnNetLog(
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
        void AddUserInfoQueriesForLobbyMembers(EOSPlayersInfoTracking& netUsersInfoTracking) const {
            // Sanity check
            if (!mLobbyProperties.IsLobbyValid()) {
                mLogger.AddWarnNetLog(
                    "NetLobbyTracking::AddUserInfoQueriesForLobbyMembers",
                    "Lobby is not valid somehow!"
                );
                return;
            }

            const std::vector<CrossPlatformIdWrapper>& membersList = mLobbyProperties.GetMembersList();
            for (const CrossPlatformIdWrapper& memberId : membersList) {
                netUsersInfoTracking.CheckIfHaveUserInfoAndQueryIfNot(mLogger, memberId);
            }
        }

        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();
        
        EOSLobbyTrackingStatus mTrackingStatus = EOSLobbyTrackingStatus::Inactive;

        EOSLobbyDetailsKeeper mDelayedJoinRequestLobby = nullptr;
        EOSLobbyProperties mLobbyProperties = {};
    };
}
