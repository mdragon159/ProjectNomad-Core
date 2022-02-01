#pragma once
#include "Math/FixedPoint.h"

namespace ProjectNomad {
    enum class PlayerSpot : uint8_t {
        INVALID,
        Player1,
        Player2,
        
        Spectator
    };
    
    struct PlayerId {
        PlayerSpot playerSpot;
        // FUTURE: Network id
    };
}
