#pragma once

#include <unordered_map>
#include <vector>

#include "NetPlayerInfo.h"
#include "NetPlayerSpotMapping.h"
#include "Network/EOS/Model/CrossPlatformIdWrapper.h"
#include "Network/EOS/Model/EOSHashFunction.h"

namespace ProjectNomad {
    struct NetLobbyInfo {
        // Basic lobby info
        bool isInLobby = false;
        bool isLocalPlayerLobbyOwner = false;
        CrossPlatformIdWrapper lobbyOwner = {};
        uint32_t lobbyMaxMembers = 0;

        // Detailed lobby members info
        std::vector<CrossPlatformIdWrapper> lobbyMemberIds = {};
        std::unordered_map<CrossPlatformIdWrapper, NetPlayerInfo, EOSHashFunction> lobbyMembersInfoMap = {};

        NetPlayerSpotMapping playerSpotMapping = {};

        bool IsIdInLobbyMembers(const CrossPlatformIdWrapper& targetId) const {
            for (const CrossPlatformIdWrapper& memberId : lobbyMemberIds) {
                if (targetId == memberId) {
                    return true;
                }
            }

            return false;
        }
    };
}
