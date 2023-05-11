#pragma once

#include "PlayerSpot.h"
#include "Network/EOS/Model/CrossPlatformIdWrapper.h"

namespace ProjectNomad {
    struct PlayerId {
        PlayerSpot playerSpot = PlayerSpot::Player1;
        CrossPlatformIdWrapper crossPlatformId = {};

        PlayerId() {}
        PlayerId(PlayerSpot playerSpot) : playerSpot(playerSpot) {}
        PlayerId(PlayerSpot playerSpot, CrossPlatformIdWrapper crossPlatformId)
        : playerSpot(playerSpot), crossPlatformId(crossPlatformId) {}
    };
}
