#pragma once

#include "NetAllPlayersInfo.h"

namespace ProjectNomad {
    /**
    * Encapsulates responsibilities for storing and retrieving player information for current play session
    **/
    class NetPlayersInfoManager {
      public:
        const NetAllPlayersInfo& GetPlayersInfo() const {
            return mPlayersInfo;
        }

        void OnLoggedIn(const CrossPlatformIdWrapper& loggedInId) {
            // Sanity check
            if (mPlayersInfo.isLoggedIn) {
                mLogger.AddWarnNetLog("NetPlayersInfoManager::OnLoggedIn", "Already logged in!");
                return;
            }

            mPlayersInfo.localPlayerId = loggedInId;
            mPlayersInfo.isLoggedIn = true;
            // Just in case clear out (currently unused) lobby data.
            //      Shouldn't be necessary given logout clears all data but nice to enforce our expectations.
            mPlayersInfo.netLobbyInfo = {};
        }

        void OnLoggedOut() {
            // Sanity check
            if (!mPlayersInfo.isLoggedIn) {
                mLogger.AddWarnNetLog("NetPlayersInfoManager::OnLoggedOut", "Not logged in, incorrect state tracking?");
                return;
            }
            
            // Current assumption is that *no* player info is valid anymore. Thus wipe it all out and use default state
            mPlayersInfo = {};
        }

        void OnLobbyJoinOrCreationResult(bool didSucceed,
                                         const EOSLobbyProperties& lobbyProperties,
                                         const EOSPlayersInfoTracking& playersInfoTracking) {
            if (!didSucceed) {
                mPlayersInfo.netLobbyInfo.isInLobby = false; // Just to be extra cautious
                return;
            }
            // Sanity checks
            if (mPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Called while already in a lobby supposedly! Likely very incorrect state tracking");
                return;
            }
            if (!lobbyProperties.IsLobbyValid()) {
                mLogger.AddWarnNetLog("Lobby properties state that lobby not valid for some reason!");
                return;
            }

            CopyLobbyInfo(lobbyProperties, playersInfoTracking);
        }

        void OnLobbyUpdated(bool didSucceed,
                            const EOSLobbyProperties& lobbyProperties,
                            const EOSPlayersInfoTracking& playersInfoTracking) {
            if (!didSucceed) {
                mPlayersInfo.netLobbyInfo.isInLobby = false; // Just to be extra cautious
                return;
            }
            // Sanity check
            if (!mPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Called while mot in a lobby supposedly! Likely very incorrect state tracking");
                return;
            }

            CopyLobbyInfo(lobbyProperties, playersInfoTracking);
        }

        void OnLobbyLeftOrDestroyed(bool didSucceed) {
            // Sanity check
            if (!mPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Called while not in a lobby supposedly! Likely very incorrect state tracking");
                return;
            }

            // Reset all lobby tracking back to defaults which represent no lobby in use atm.
            //      Note that this is a nice easy catch-all data reset, such as resetting potential lobby lock status.
            mPlayersInfo.netLobbyInfo = {};
        }

        /**
        * Lock key lobby state from any potential changes. Two key uses cases:
        *       1. TODO: No new members should be able to join the lobby (successfully) once match starts
        *       2. EOS may frequently do LobbyUpdate callbacks which may change player spot mapping even if there were
        *               no real changes (like members joining or leaving). Given data like player spot mapping needs to
        *               be consistent throughout the life of a match, we need to prevent that data being changed during a match.
        **/
        void LockLobby() {
            if (!mPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Called while not in a lobby supposedly! Likely very incorrect state tracking");
                return;
            }
            if (mPlayersInfo.netLobbyInfo.playerSpotMapping.IsLocked()) {
                mLogger.AddWarnNetLog("Called while already locked");
                return;
            }
            if (!mPlayersInfo.netLobbyInfo.playerSpotMapping.IsMappingSet()) { // Player spot mapping should always be set before locking it down
                mLogger.AddWarnNetLog("Called while player spot mapping not yet set. Unexpected circumstances! Is this expected now?");
                return;
            }

            // For now only player spot mapping is locked as not sure yet how should go about "locking" EOS lobby itself
            mPlayersInfo.netLobbyInfo.playerSpotMapping.SetLock(true);
        }
        void UnlockLobby() {
            if (!mPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Called while not in a lobby supposedly! Likely very incorrect state tracking");
                return;
            }
            if (!mPlayersInfo.netLobbyInfo.playerSpotMapping.IsLocked()) {
                mLogger.AddWarnNetLog("Called while not locked");
                return;
            }

            // For now only player spot mapping is locked as not sure yet how should go about "locking" EOS lobby itself
            mPlayersInfo.netLobbyInfo.playerSpotMapping.SetLock(false);
        }

        void UpdateLobbyMembersGeneralInfo(const EOSPlayersInfoTracking& playersInfoTracking) {
            // If not in a lobby then nothing to update atm.
            //      This is an expected valid edge case that may occur when, say, looking up a lobby member's user info
            //      but leaving the lobby before that EOS user info lookup completes.
            if (!mPlayersInfo.netLobbyInfo.isInLobby) {
                return;
            }

            // Empty out the cached detailed members info map as may be outdated
            mPlayersInfo.netLobbyInfo.lobbyMembersInfoMap.clear();
            
            // Loop through all lobby members and try setting their info
            for (const CrossPlatformIdWrapper& memberId : mPlayersInfo.netLobbyInfo.lobbyMemberIds) {
                NetPlayerInfo dataToInsert = {};
                dataToInsert.playerId = memberId; // Redundant but personal taste for struct to be somewhat independently usable

                // Retrieve relevant info (if available) from EOS wrapper side
                const EOSPlayerInfo& eosPlayerInfo = playersInfoTracking.GetCachedPlayerInfoIfAny(mLogger, memberId);
                dataToInsert.isWaitingForDisplayName = !eosPlayerInfo.hasInfo;
                if (!dataToInsert.isWaitingForDisplayName) {
                    dataToInsert.displayName = eosPlayerInfo.sanitizedDisplayName;
                }
                
                mPlayersInfo.netLobbyInfo.lobbyMembersInfoMap.insert({memberId, dataToInsert});
            }
        }
        void SetPlayerSpotMappingFromHost(const std::vector<CrossPlatformIdWrapper>& allPlayerIdsInOrder) {
            // Sanity check: Host should *not* be sending a player spot mapping while lobby is locked, as that may
            //               break the match itself. (Eg, Player1 and Player2 players being suddenly switched mid-game)
            if (mPlayersInfo.netLobbyInfo.playerSpotMapping.IsLocked()) {
                mLogger.AddWarnNetLog("Lobby is already locked and host should not be trying to update mapping!");
                return;
            }
            
            mPlayersInfo.netLobbyInfo.playerSpotMapping.SetMapping(
                mLogger, mPlayersInfo.localPlayerId, allPlayerIdsInOrder
            );
        }

      private:
        void CopyLobbyInfo(const EOSLobbyProperties& lobbyProperties, const EOSPlayersInfoTracking& playersInfoTracking) {
            // FUTURE: Much of this is straight copied from EOSWrapper lobby data. Maybe we should just reference that mostly?
            mPlayersInfo.netLobbyInfo.isInLobby = true;
            
            // Copy straightforward lobby data so easily accessible
            mPlayersInfo.netLobbyInfo.isLocalPlayerLobbyOwner = lobbyProperties.IsLocalPlayerLobbyOwner();
            mPlayersInfo.netLobbyInfo.lobbyOwner = lobbyProperties.GetLobbyOwner();
            mPlayersInfo.netLobbyInfo.lobbyMaxMembers  = lobbyProperties.GetMaxMembers();
            mPlayersInfo.netLobbyInfo.lobbyMemberIds = lobbyProperties.GetMembersList(); // Expecting this to be very smol array of very smol types

            // Do further post-processing on top of the initial copied data
            UpdateLobbyMembersGeneralInfo(playersInfoTracking);
            CreatePlayerSpotMappingIfNecessary();
        }
        void CreatePlayerSpotMappingIfNecessary() {
            // Sanity check assumptions
            if (!mPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Called while currently not in a lobby!");
                return;
            }
            if (mPlayersInfo.netLobbyInfo.lobbyMemberIds.empty()) {
                mLogger.AddWarnNetLog(
                    "Lobby members list is empty. Expected this to be setup before this method is called!"
                );
                return;
            }

            // If not host, then nothing to do as only host dictates the player spot mapping.
            //      This is necessary as order within list of lobby members from EOS differs per player.
            //      Thus, every player needs the same ordered list of players which needs to come from one player- the host.
            if (!mPlayersInfo.netLobbyInfo.isLocalPlayerLobbyOwner) {
                return;
            }
            // If player spot mapping locked, then ignore any potential lobby updates to assure match isn't broken.
            //      Eg, EOS may change lobby members order at any time which would break the match's player spot mapping.
            if (mPlayersInfo.netLobbyInfo.playerSpotMapping.IsLocked()) {
                return;
            }

            mPlayersInfo.netLobbyInfo.playerSpotMapping.SetMapping(
                mLogger, mPlayersInfo.localPlayerId, mPlayersInfo.netLobbyInfo.lobbyMemberIds
            );
        }
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();
        NetAllPlayersInfo mPlayersInfo = {};
    };
}
