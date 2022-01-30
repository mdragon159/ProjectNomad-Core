#pragma once

namespace ProjectNomad {
    enum class NetStatus : uint8_t { NotInitialized, Initialized, LoggedIn, TryingToConnect, ConnectedToOtherPlayer };
}
