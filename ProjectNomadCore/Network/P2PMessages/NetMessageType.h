#pragma once

#include <cstdint>

namespace ProjectNomad {
    // FUTURE: Consider using template metaprogramming magic to automatically generate message id
    enum class NetMessageType : uint8_t {
        INVALID,
        TryConnect,
        AcceptConnection,
        InputUpdate,
        PlayerSpotMapping,

        PrepareLobbyStartMatch,
        ConfirmedLobbyStartMatch,
        LoadMap,
        FinishedMapLoad,
        StartGameplay,

        ENUM_COUNT // https://stackoverflow.com/a/14989325/3735890
    };
}
