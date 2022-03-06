#pragma once
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

        template <class Archive>
        void serialize(Archive& ar, std::uint32_t const version) {
            ar(moveForward, moveRight, mouseTurn, mouseLookUp, controllerTurn, controllerLookUp, isJumpPressed,
                isBlockPressed, isDodgePressed, isGrapplePressed, isGrappleAimPressed, isAttackPressed, isSecondaryAttackPressed, isCrouchPressed);
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
