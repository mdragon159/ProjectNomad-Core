#pragma once

namespace ProjectNomad {
    // FUTURE: Consider using template metaprogramming magic to automatically generate message id
    enum class NetMessageType : uint8_t {
        INVALID,
        InputsUpdate
    };

    // All messages sent to other players are expected to extend from this type
    struct BaseNetMessage {
        // ASSUMING that this enum will always be at the beginning of any given NetMessage
        // Pretty sure this will always be the case when BaseNetMessage is the base type, but do need to verify in future
        NetMessageType messageType;
    };
}
