#pragma once

namespace ProjectNomad {
    // Enum matching EOS_EPacketReliability. Useful to not expose EOS dependencies outside EOSWrapper
    enum class PacketReliability : uint8_t {
        UnreliableUnordered,    // UDP: Packets will only be sent once and may be received out of order
        ReliableUnordered,      // TCP?: Packets may be sent multiple times and may be received out of order
        ReliableOrdered,        // TCP: Packets may be sent multiple times and will be received in order
    };
}
