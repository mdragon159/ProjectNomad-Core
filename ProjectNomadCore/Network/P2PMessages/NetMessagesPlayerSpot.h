#pragma once

#include "BaseNetMessage.h"

namespace ProjectNomad {
    // This represents an array of CrossPlatformIdWrapper ids in order of PlayerSpots
    using RawPlayerIdArrayWithoutExtraSpot = char[EOS_PRODUCTUSERID_MAX_LENGTH]; // Mentally easier on me to use an "inner" alias with a 2D array
    using OrderedRawPlayerIdArray = RawPlayerIdArrayWithoutExtraSpot[PlayerSpotHelpers::kMaxPlayerSpots];
    
    struct PlayerSpotMappingMessage : BaseNetMessage {
        uint8_t totalPlayers = 0;
        OrderedRawPlayerIdArray rawPlayerSpotMapping = {};

        PlayerSpotMappingMessage() : BaseNetMessage(NetMessageType::PlayerSpotMapping) {}
    };
}
