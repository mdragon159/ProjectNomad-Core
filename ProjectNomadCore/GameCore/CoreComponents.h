#pragma once

#include <CRCpp/CRC.h>

#include "Math/FQuatFP.h"
#include "Math/FVectorFP.h"
#include "Physics/Model/FCollider.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    // CoreComponents.h simply contains definition of components which both ProjectNomadCore and the main ProjectNomad projects need

    /**
    * Anything that has a location and rotation aside from special cases (eg, static level entities) should have this comp
    **/
    struct TransformComponent {
        FVectorFP location = FVectorFP::Zero();
        FQuatFP rotation = FQuatFP::identity();

        // Represents direction that entity is facing, *assuming* +x axis is intended to be forward direction
        FVectorFP getForwardDirection() const {
            return rotation * FVectorFP::Forward();
        }
        
        void CalculateCRC32(uint32_t& resultThusFar) const {
            location.CalculateCRC32(resultThusFar);
            rotation.CalculateCRC32(resultThusFar);
        }
    };

    struct PhysicsComponent {
        FFixedPoint mass = FFixedPoint(100);
        FVectorFP velocity = FVectorFP::Zero();

        bool HasAnyVelocity() const {
            return velocity.GetLengthSquared() != fp{0};
        }

        bool HasAnyHorizontalVelocity() const {
            return velocity.x != fp{0} || velocity.y != fp{0};
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            mass.CalculateCRC32(resultThusFar);
            velocity.CalculateCRC32(resultThusFar);
        }
    };

    struct DynamicColliderComponent {
        FCollider collider = {};

        void CalculateCRC32(uint32_t& resultThusFar) const {
            collider.CalculateCRC32(resultThusFar);
        }
    };

    struct StaticColliderComponent {
        FCollider collider = {};
        
        void CalculateCRC32(uint32_t& resultThusFar) const {
            collider.CalculateCRC32(resultThusFar);
        }
    };

    struct HitstopComponent {
        FrameType startingFrame = 0;
        FrameType totalLength = 15;

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&startingFrame, sizeof(startingFrame), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&totalLength, sizeof(totalLength), CRC::CRC_32(), resultThusFar);
        }
    };

    /**
    * Simply marks if entity is "invulnerable" (ie, that generally cannot be interacted with)
    * TODO: Actually use this for all combat situations (like hit checks and grapple checks)
    * FUTURE: MAYBE use this for certain physics situations (like optionally when checking for collisions w/ dynamic entities)
    **/
    struct InvulnerableFlagComponent {
        bool throwaway = false;

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&throwaway, sizeof(throwaway), CRC::CRC_32(), resultThusFar);
        }
    };
}
