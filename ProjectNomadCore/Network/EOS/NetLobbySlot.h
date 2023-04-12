#pragma once
#include "EOSLobbyProperties.h"

namespace ProjectNomad {
    // Identifier for which lobby is intended to be used for a specific lobby API call.
    // Why not just pass "real" data around? Just cuz passing in an actual void* to EOS SDK is far too easy to mess up.
    enum class NetLobbySlot : uint8_t {
        MainMatchLobby,
    };
}
