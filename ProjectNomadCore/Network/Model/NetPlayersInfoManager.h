#pragma once

#include "NetPlayersInfo.h"

namespace ProjectNomad {
    /**
    * Encapsulates responsibilities for storing and retrieving player information for current play session
    **/
    class NetPlayersInfoManager {
      public:
        const NetPlayersInfo& GetPlayersInfo() const {
            return mPlayersInfo;
        }

        void OnLoggedIn(const CrossPlatformIdWrapper& loggedInId) {
            // Sanity check
            if (mPlayersInfo.isLoggedIn) {
                mLogger.addWarnNetLog("NetPlayersInfoManager::OnLoggedIn", "Already logged in!");
                return;
            }

            mPlayersInfo.localPlayerId = loggedInId;
            mPlayersInfo.isLoggedIn = true;
        }

        void OnLoggedOut() {
            // Sanity check
            if (!mPlayersInfo.isLoggedIn) {
                mLogger.addWarnNetLog("NetPlayersInfoManager::OnLoggedOut", "Not logged in, incorrect state tracking?");
                return;
            }
            
            // Current assumption is that *no* player info is valid anymore. Thus wipe it all out and use default state
            mPlayersInfo = {};
        }

        void OnLobbyJoinOrCreationResult(bool didSucceed, const EOSLobbyProperties& lobbyProperties) {
            if (!didSucceed) {
                mPlayersInfo.isInLobby = false; // Just to be extra cautious
                return;
            }
            // Sanity checks
            if (mPlayersInfo.isInLobby) {
                mLogger.addWarnNetLog(
                    "NetPlayersInfoManager::OnLobbyJoinOrCreationResult",
                    "Called while already in a lobby supposedly! Likely very incorrect state tracking"
                );
                return;
            }
            if (!lobbyProperties.IsLobbyValid()) {
                mLogger.addWarnNetLog(
                    "NetPlayersInfoManager::OnLobbyJoinOrCreationResult",
                    "Lobby properties state that lobby not valid for some reason!"
                );
                return;
            }

            CopyLobbyInfo(lobbyProperties);
            mPlayersInfo.isInLobby = true;
        }

        void OnLobbyUpdated(bool didSucceed, const EOSLobbyProperties& lobbyProperties) {
            if (!didSucceed) {
                mPlayersInfo.isInLobby = false; // Just to be extra cautious
                return;
            }
            // Sanity check
            if (!mPlayersInfo.isInLobby) {
                mLogger.addWarnNetLog(
                    "NetPlayersInfoManager::OnLobbyUpdated",
                    "Called while mot in a lobby supposedly! Likely very incorrect state tracking"
                );
                return;
            }

            CopyLobbyInfo(lobbyProperties);
        }

        void OnLobbyLeftOrDestroyed(bool didSucceed) {
            // Sanity check
            if (!mPlayersInfo.isInLobby) {
                mLogger.addWarnNetLog(
                    "NetPlayersInfoManager::OnLobbyLeftOrDestroyed",
                    "Called while not in a lobby supposedly! Likely very incorrect state tracking"
                );
                return;
            }

            // Clear up the following as no guarantee that consuming code will properly check lobby state before info usage
            mPlayersInfo.lobbyMembersList.clear();

            // Mark lobby tracking as inactive/unused
            mPlayersInfo.isInLobby = false;
        }

      private:
        void CopyLobbyInfo(const EOSLobbyProperties& lobbyProperties) {
            // FUTURE: Much of this is straight copied from EOSWrapper lobby data. Maybe we should just reference that mostly?

            // Copy straightforward lobby data so easily accessible
            mPlayersInfo.isLocalPlayerLobbyOwner = lobbyProperties.IsLocalPlayerLobbyOwner();
            mPlayersInfo.lobbyMaxMembers  = lobbyProperties.GetMaxMembers();
            mPlayersInfo.lobbyMembersList = lobbyProperties.GetMembersList(); // Expecting this to be very smol array of very smol types

            // Do further post-processing on top of the initial copied data
            TryDetermineLocalPlayerSpot();
        }
        void TryDetermineLocalPlayerSpot() {
            // Sanity check: Validate not currently unsupported number of members in lobby
            uint32_t finalValidEnumVal = static_cast<uint32_t>(PlayerSpot::ENUM_COUNT) - 1;
            if (mPlayersInfo.lobbyMembersList.size() > finalValidEnumVal) {
                mLogger.addWarnNetLog(
                    "NetPlayersInfoManager::TryDetermineLocalPlayerSpot",
                    "Lobby members size is larger than supported size. Max supported size: " +
                    std::to_string(finalValidEnumVal) + ", lobby member size: " + std::to_string(mPlayersInfo.lobbyMembersList.size())
                );
                return;
            }

            uint8_t validatedMemberSize = mPlayersInfo.lobbyMembersList.size();
            
            // Determine local player's spot out of members list. Expecting this to be commonly used
            bool foundLocalPlayerSpot = false;
            uint8_t spotCheck;
            for (spotCheck = 0; spotCheck < validatedMemberSize; spotCheck++) {
                if (mPlayersInfo.lobbyMembersList[spotCheck].getAccountId() == mPlayersInfo.localPlayerId.getAccountId()) {
                    foundLocalPlayerSpot = true;
                    break;
                }
            }

            if (!foundLocalPlayerSpot) {
                mLogger.addWarnNetLog(
                    "NetPlayersInfoManager::TryDetermineLocalPlayerSpot",
                    "Did not find local player in lobby members list!"
                );
                return;
            }

            mPlayersInfo.localPlayerSpot = static_cast<PlayerSpot>(spotCheck);
        }
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();
        NetPlayersInfo mPlayersInfo = {};
    };
}
