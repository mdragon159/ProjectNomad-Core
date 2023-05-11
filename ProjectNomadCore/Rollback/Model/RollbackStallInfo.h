#pragma once

#include "GameCore/PlayerSpot.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    struct RollbackStallPlayerInfo {
        // Bit of debug info to help with identifying stalling player
        PlayerSpot waitingOnPlayer = PlayerSpot::Player1;
        FrameType lastFrameReceived = 0;
    };

    using FlexStallPlayerInfoArray = FlexArray<RollbackStallPlayerInfo, PlayerSpotHelpers::kMaxPlayerSpots>; // No need for a std::vector
    struct RollbackStallInfo {
        static RollbackStallInfo NoStall() { // Shorthand method for "should not stall" result
            return {}; // Defaults to no stall result
        }
        static RollbackStallInfo WithStall(const FlexStallPlayerInfoArray& waitingOnPlayers) {
            RollbackStallInfo result = {};
            result.shouldStall = true;
            result.waitingOnPlayers = waitingOnPlayers;
            return result;
        }
        
        bool shouldStall = false;
        FlexStallPlayerInfoArray waitingOnPlayers = {};
    };
}
