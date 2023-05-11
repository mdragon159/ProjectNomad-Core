#pragma once

#include "NetLobbyInfo.h"
#include "Network/EOS/Model/CrossPlatformIdWrapper.h"

namespace ProjectNomad {
    /**
    * Encapsulates player-related info for "user" side of netcode
    **/
    struct NetAllPlayersInfo {
        // Login-related data
        bool isLoggedIn = false;
        CrossPlatformIdWrapper localPlayerId = {};
        
        NetLobbyInfo netLobbyInfo = {};
    };
}
