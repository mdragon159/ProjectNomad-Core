#pragma once

#include <CRCpp/CRC.h>

#include "Math/FPQuat.h"
#include "Math/FPVector.h"
#include "Physics/Collider.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    // CoreComponents.h simply contains definition of components which both ProjectNomadCore and the main ProjectNomad projects need
    
    struct TransformComponent {
        FPVector location;
        FPQuat rotation;

        void CalculateCRC32(uint32_t& resultThusFar) const {
            location.CalculateCRC32(resultThusFar);
            rotation.CalculateCRC32(resultThusFar);
        }

        // Represents direction that entity is facing, *assuming* +x axis is intended to be forward direction
        FPVector getForwardDirection() const {
            return rotation * FPVector::forward();
        }
    };

    struct PhysicsComponent {
        uint16_t mass = 100;
        FPVector velocity;

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&mass, sizeof(mass), CRC::CRC_32(), resultThusFar);
            velocity.CalculateCRC32(resultThusFar);
        }
    };

    struct DynamicColliderComponent {
        Collider collider;

        void CalculateCRC32(uint32_t& resultThusFar) const {
            collider.CalculateCRC32(resultThusFar);
        }
    };

    struct StaticColliderComponent {
        Collider collider;
        
        void CalculateCRC32(uint32_t& resultThusFar) const {
            collider.CalculateCRC32(resultThusFar);
        }
    };

    struct HitfreezeComponent {
        FrameType startingFrame = 0;
        FrameType totalLength = 15;

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&startingFrame, sizeof(startingFrame), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&totalLength, sizeof(totalLength), CRC::CRC_32(), resultThusFar);
        }
    };
}
