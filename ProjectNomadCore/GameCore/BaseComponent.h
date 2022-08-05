#pragma once

#include <cstdint>
#include <CRCpp/CRC.h>

namespace ProjectNomad {
    struct BaseComponent {
        virtual ~BaseComponent() {}

        /**
        * Calculate CRC32 checksum via directly processing each value. This is intended to avoid creating a checksum
        * of padding bits. See following for more info: https://stackoverflow.com/q/19545557/3735890
        * @param resultThusFar - Input-output parameter for CRC32 calculation 
        **/
        virtual void CalculateCRC32(uint32_t& resultThusFar) = 0;
    };
}
