#pragma once

#include <CRCpp/CRC.h>

#include "CommandSetList.h"
#include "GameplayInteractiveUIChoice.h"
#include "Math/FixedPoint.h"
#include "Math/FPVector.h"
#include "Math/FPQuat.h"

namespace ProjectNomad {
    struct PlayerInput {
        // fp values represent axis inputs which are in range [-1, 1].
        // FUTURE: Compress all values to take up less space.

        FPVector camPosition = {};
        FPQuat camRotation = {};
        
        fp moveForward = fp{0};
        fp moveRight = fp{0};

        GameplayInteractiveUIChoice uiChoice =  GameplayInteractiveUIChoice::None;
        
        // Raw button inputs would take up less space, but the underlying array of bools won't take up much more space anyways.
        // Nice to abstract away independent buttons vs actual action commands
        CommandSetList commandInputs = {};

        void CalculateCRC32(uint32_t& resultThusFar) const {
            camPosition.CalculateCRC32(resultThusFar);
            camRotation.CalculateCRC32(resultThusFar);
            
            resultThusFar = CRC::Calculate(&moveForward, sizeof(moveForward), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&moveRight, sizeof(moveRight), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&uiChoice, sizeof(uiChoice), CRC::CRC_32(), resultThusFar);
            
            commandInputs.CalculateCRC32(resultThusFar);
        }
    };

    // Explicitly define equality operators as fp doesn't support <=>, may need to update library
    inline bool operator==(const PlayerInput& lhs, const PlayerInput& rhs) {
        return  lhs.camPosition == rhs.camPosition &&
                lhs.camRotation == rhs.camRotation &&
                lhs.commandInputs == rhs.commandInputs &&
                lhs.moveForward == rhs.moveForward &&
                lhs.moveRight == rhs.moveRight;
    }
    inline bool operator!=(const PlayerInput& lhs, const PlayerInput& rhs) {
        return !(lhs == rhs);
    }
}
