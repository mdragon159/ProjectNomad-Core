#pragma once

namespace ProjectNomad {
    enum class EOSWrapperStatus : uint8_t {
        NotInitialized, Initialized, LoggedIn, TryingToConnect, ConnectedToOtherPlayer
    };
}
