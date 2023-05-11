#pragma once

#include <string>

#include "Network/EOS/Model/CrossPlatformIdWrapper.h"

namespace ProjectNomad {
    struct NetPlayerInfo {
        CrossPlatformIdWrapper playerId = {};
        
        bool isWaitingForDisplayName = false;
        std::string displayName = "";
    };
}
