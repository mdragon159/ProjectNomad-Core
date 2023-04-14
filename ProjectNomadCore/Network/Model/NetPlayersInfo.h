#pragma once

#include <vector>

#include "GameCore/PlayerId.h"
#include "Network/EOS/Model/CrossPlatformIdWrapper.h"

namespace ProjectNomad {
    /**
    * Encapsulates player-related info for "user" side of netcode
    **/
    struct NetPlayersInfo {
        // Login-related data
        bool isLoggedIn = false;
        CrossPlatformIdWrapper localPlayerId = {};
        
        // Lobby related data
        bool isInLobby = false;
        bool isLocalPlayerLobbyOwner = false;
        uint32_t lobbyMaxMembers = 0;
        std::vector<CrossPlatformIdWrapper> lobbyMembersList = {};
        PlayerSpot localPlayerSpot = PlayerSpot::Player1; // Nice to not need to search through list every time for this
    };
}
