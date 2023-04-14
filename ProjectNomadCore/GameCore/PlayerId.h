#pragma once

#include "Network/EOS/Model/CrossPlatformIdWrapper.h"

namespace ProjectNomad {
    enum class PlayerSpot : uint8_t {
        Player1,
        Player2,
        Player3,
        Player4,

        ENUM_COUNT // https://stackoverflow.com/a/14989325/3735890
    };
    
    struct PlayerId {
        PlayerSpot playerSpot = PlayerSpot::Player1;
        CrossPlatformIdWrapper crossPlatformId = {};

        PlayerId() {}
        PlayerId(PlayerSpot playerSpot) : playerSpot(playerSpot) {}
        PlayerId(PlayerSpot playerSpot, CrossPlatformIdWrapper crossPlatformId)
        : playerSpot(playerSpot), crossPlatformId(crossPlatformId) {}
    };
}
