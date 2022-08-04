#pragma once
#include <cstdint>

namespace ProjectNomad {
    struct BaseSnapshot {
        virtual ~BaseSnapshot() = default;
        virtual uint32_t CalculateChecksum() const = 0;
    };
}
