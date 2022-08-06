#pragma once

#include <CRCpp/CRC.h>
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

        bool isInteractPressed = false;
        bool isJumpPressed = false;
        bool isDodgePressed = false;
        bool isBlockPressed = false;
        bool isGrapplePressed = false;
        bool isGrappleAimPressed = false;
        bool isAttackPressed = false;
        bool isSecondaryAttackPressed = false;
        bool isCrouchPressed = false;
        bool isSwitchWeaponPressed = false;

        template <class Archive>
        void serialize(Archive& ar, std::uint32_t const version) {
            ar(moveForward, moveRight, mouseTurn, mouseLookUp, controllerTurn, controllerLookUp, isJumpPressed,
                isBlockPressed, isDodgePressed, isGrapplePressed, isGrappleAimPressed, isAttackPressed,
                isSecondaryAttackPressed, isCrouchPressed, isSwitchWeaponPressed);
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&moveForward, sizeof(moveForward), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&moveRight, sizeof(moveRight), CRC::CRC_32(), resultThusFar);
            
            resultThusFar = CRC::Calculate(&mouseTurn, sizeof(mouseTurn), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&mouseLookUp, sizeof(mouseLookUp), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&controllerTurn, sizeof(controllerTurn), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&controllerLookUp, sizeof(controllerLookUp), CRC::CRC_32(), resultThusFar);

            resultThusFar = CRC::Calculate(&isInteractPressed, sizeof(isInteractPressed), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&isJumpPressed, sizeof(isJumpPressed), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&isDodgePressed, sizeof(isDodgePressed), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&isBlockPressed, sizeof(isBlockPressed), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&isGrapplePressed, sizeof(isGrapplePressed), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&isGrappleAimPressed, sizeof(isGrappleAimPressed), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&isAttackPressed, sizeof(isAttackPressed), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&isSecondaryAttackPressed, sizeof(isSecondaryAttackPressed), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&isCrouchPressed, sizeof(isCrouchPressed), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&isSwitchWeaponPressed, sizeof(isSwitchWeaponPressed), CRC::CRC_32(), resultThusFar);
        }
    };

    // Explicitly define equality operators as pre-C++20 can't auto-generate this: https://stackoverflow.com/a/5740505/3735890
    inline bool operator==(const PlayerInput& lhs, const PlayerInput& rhs) {
        return  lhs.isJumpPressed == rhs.isJumpPressed &&
                lhs.isDodgePressed == rhs.isDodgePressed &&
                lhs.isBlockPressed == rhs.isBlockPressed &&
                lhs.isGrapplePressed == rhs.isGrapplePressed &&
                lhs.isGrappleAimPressed == rhs.isGrappleAimPressed &&
                lhs.isAttackPressed == rhs.isAttackPressed &&
                lhs.isSecondaryAttackPressed == rhs.isSecondaryAttackPressed &&
                lhs.isCrouchPressed == rhs.isCrouchPressed &&
                lhs.isSwitchWeaponPressed == rhs.isSwitchWeaponPressed &&
                    
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
