#pragma once

#include <CRCpp/CRC.h>

#include "CommandSetList.h"
#include "InputCommand.h"
#include "Math/FixedPoint.h"

namespace ProjectNomad {
    struct PlayerInput {
        // fp values represent axis inputs which are in range [-1, 1]
        
        fp moveForward = fp{0};
        fp moveRight = fp{0};
        
        fp mouseTurn = fp{0};
        fp mouseLookUp = fp{0};
        fp controllerTurn = fp{0};
        fp controllerLookUp = fp{0};

        // Raw button inputs would take up less space, but the underlying array of bools won't take up much more space anyways.
        // Nice to abstract away independent buttons vs actual action commands
        CommandSetList commandInputs = {};

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&moveForward, sizeof(moveForward), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&moveRight, sizeof(moveRight), CRC::CRC_32(), resultThusFar);
            
            resultThusFar = CRC::Calculate(&mouseTurn, sizeof(mouseTurn), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&mouseLookUp, sizeof(mouseLookUp), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&controllerTurn, sizeof(controllerTurn), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&controllerLookUp, sizeof(controllerLookUp), CRC::CRC_32(), resultThusFar);

            commandInputs.CalculateCRC32(resultThusFar);
        }
    };

    // Explicitly define equality operators as fp doesn't support <=>, may need to update library
    inline bool operator==(const PlayerInput& lhs, const PlayerInput& rhs) {
        return  lhs.commandInputs == rhs.commandInputs &&
                lhs.moveForward == rhs.moveForward &&
                lhs.moveRight == rhs.moveRight &&
                lhs.mouseTurn == rhs.mouseTurn &&
                lhs.mouseLookUp == rhs.mouseLookUp &&
                lhs.controllerTurn == rhs.controllerTurn &&
                lhs.controllerLookUp == rhs.controllerLookUp;
    }
    inline bool operator!=(const PlayerInput& lhs, const PlayerInput& rhs) {
        return !(lhs == rhs);
    }
}
