#pragma once

#include <CRCpp/CRC.h>

#include "CommandSetList.h"
#include "GameplayInteractiveUIChoice.h"
#include "Math/FixedPoint.h"
#include "Math/FPVector.h"
#include "Math/FPQuat.h"

namespace ProjectNomad {
    /**
    * Defines input necessary to control any entity. Eg, this is the player input sent in Multiplayer.
    *
    * NOTE: If this is updated, then make sure to also update any file I/O (like replay file serialization) or
    *       related network messages.
    **/
    struct CharacterInput {
        // fp values represent axis inputs which are in range [-1, 1].
        // FUTURE: Compress all values to take up less space.

        // TODO: Get rid of cam pos entirely!
        //      Copy other melee-games-with-shooting such as Wo Long, where aiming goes into over-the-shoulder aim mode,
        //      where aim pos is derived from rotation. Can even make grapple-aim-startup anim frames which mask the discontinuity.
        //      Aside from smaller packet size, this helps with "prediction"! (Rollback frame count and replay compression)
        FPVector camPosition = {};
        FPQuat camRotation = {}; // TODO: Compress down into  8bits per fp
        
        fp moveForward = fp{0}; // TODO: Compress down into  8bits per fp
        fp moveRight = fp{0}; // TODO: Compress down into  8bits per fp

        GameplayInteractiveUIChoice uiChoice =  GameplayInteractiveUIChoice::None;
        
        // Raw button inputs would take up less space, but the underlying array of bools won't take up much more space anyways.
        // Nice to abstract away independent buttons vs actual action commands
        CommandSetList commandInputs = {}; // TODO: Compress down into 16bits rather than 32bits

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
    inline bool operator==(const CharacterInput& lhs, const CharacterInput& rhs) {
        return  lhs.camPosition == rhs.camPosition &&
                lhs.camRotation == rhs.camRotation &&
                lhs.moveForward == rhs.moveForward &&
                lhs.moveRight == rhs.moveRight &&
                lhs.commandInputs == rhs.commandInputs &&
                lhs.uiChoice == rhs.uiChoice;
    }
    inline bool operator!=(const CharacterInput& lhs, const CharacterInput& rhs) {
        return !(lhs == rhs);
    }
}
