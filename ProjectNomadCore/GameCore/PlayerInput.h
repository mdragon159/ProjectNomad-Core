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

        bool isJumpPressed = false;
        bool isBlockDodgePressed = false;
        bool isGrapplePressed = false;
        bool isGrappleAimPressed = false;
        bool isAttackPressed = false;
        bool isCrouchPressed = false;

        template <class Archive>
        void serialize(Archive& ar, std::uint32_t const version) {
            ar(moveForward, moveRight, mouseTurn, mouseLookUp, controllerTurn, controllerLookUp,
                isJumpPressed, isBlockDodgePressed, isGrapplePressed, isGrappleAimPressed, isAttackPressed, isCrouchPressed);
        }
    };

    // Explicitly define equality operators as pre-C++20 can't auto-generate this: https://stackoverflow.com/a/5740505/3735890
    inline bool operator==(const PlayerInput& lhs, const PlayerInput& rhs) {
        return  lhs.isJumpPressed == rhs.isJumpPressed &&
                lhs.isBlockDodgePressed == rhs.isBlockDodgePressed &&
                lhs.isGrapplePressed == rhs.isGrapplePressed &&
                lhs.isGrappleAimPressed == rhs.isGrappleAimPressed &&
                lhs.isAttackPressed == rhs.isAttackPressed &&
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
