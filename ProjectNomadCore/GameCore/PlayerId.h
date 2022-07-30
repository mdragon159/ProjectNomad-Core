#pragma once

#include "Network/EOS/CrossPlatformIdWrapper.h"

namespace ProjectNomad {
    enum class PlayerSpot : uint8_t {
        INVALID,
        Player1,
        Player2,
        Player3,
        Player4,
        
        Spectator
    };
    
    struct PlayerId {
        PlayerSpot playerSpot = PlayerSpot::INVALID;
        CrossPlatformIdWrapper crossPlatformId = nullptr;

        PlayerId() {}
        PlayerId(PlayerSpot playerSpot) : playerSpot(playerSpot) {}
        PlayerId(PlayerSpot playerSpot, CrossPlatformIdWrapper crossPlatformId)
        : playerSpot(playerSpot), crossPlatformId(crossPlatformId) {}
    };
}
