#pragma once

#include "GameCore/PlayerId.h"

namespace ProjectNomad {
    struct RollbackSessionInfo {
        bool isNetworkedMPSession = false; // May have multiple players while NOT using multiplayer during, say, replays
        
        // Expected to be no less than 0 and no greater than max number in PlayerSpot
        uint8_t totalPlayers = 1;

        // Which "spot" is locally controlled player using? 
        PlayerId localPlayerId = PlayerSpot::Player1;
    };
}
