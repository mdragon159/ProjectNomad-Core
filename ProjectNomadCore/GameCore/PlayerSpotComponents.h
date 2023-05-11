#pragma once

#include "CRCpp/CRC.h"

namespace ProjectNomad {
    // Create a component per player spot so don't need an explicit player spot to entity id mapping.
    //      Decided to take on more manual one-time work rather than dynamically maintain a runtime mapping as this
    //      should be more robust as more so relying on underlying EnTT system to maintain mapping.
    // Pros and cons to either approach, this should be "good enough" for now

    // Define a base component type for easy static asserts with templated metaprogramming.
    //      Also saves copy-pasting the identical innards
    struct PlayerSpotBaseComponent {
        bool throwaway = false; // Define some data as EnTT seems to not work well with empty types (at least as of 2022)

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&throwaway, sizeof(throwaway), CRC::CRC_32(), resultThusFar);
        }
    };
    
    struct PlayerSpot1Component : PlayerSpotBaseComponent {};
    struct PlayerSpot2Component : PlayerSpotBaseComponent {};
    struct PlayerSpot3Component : PlayerSpotBaseComponent {};
    struct PlayerSpot4Component : PlayerSpotBaseComponent {};
}
