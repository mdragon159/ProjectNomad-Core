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
}
