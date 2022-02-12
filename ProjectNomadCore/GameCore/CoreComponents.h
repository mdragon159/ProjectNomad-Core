#pragma once

#include "Math/FPQuat.h"
#include "Math/FPVector.h"
#include "Physics/Collider.h"

namespace ProjectNomad {
    // CoreComponents.h simply contains definition of components which both ProjectNomadCore and the main ProjectNomad projects need
    
    struct TransformComponent {
        FPVector location;
        FPQuat rotation;

        // Represents direction that entity is facing, *assuming* +x axis is intended to be forward direction
        FPVector getForwardDirection() const {
            return rotation * FPVector::forward();
        }
    };

    struct PhysicsComponent {
        uint16_t mass = 100;
        FPVector velocity;
    };

    struct DynamicColliderComponent {
        Collider collider;
    };

    struct StaticColliderComponent {
        Collider collider;
    };

    struct HitfreezeComponent {
        uint32_t startingFrame;
        uint32_t totalLength = 15;
    };
}
