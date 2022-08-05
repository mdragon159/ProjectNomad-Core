#pragma once

#include "BaseComponent.h"
#include "Math/FPQuat.h"
#include "Math/FPVector.h"
#include "Physics/Collider.h"

namespace ProjectNomad {
    // CoreComponents.h simply contains definition of components which both ProjectNomadCore and the main ProjectNomad projects need
    
    struct TransformComponent : BaseComponent {
        FPVector location;
        FPQuat rotation;

        ~TransformComponent() override {}

        void CalculateCRC32(uint32_t& resultThusFar) override {
            location.CalculateCRC32(resultThusFar);
            rotation.CalculateCRC32(resultThusFar);
        }

        // Represents direction that entity is facing, *assuming* +x axis is intended to be forward direction
        FPVector getForwardDirection() const {
            return rotation * FPVector::forward();
        }
    };

    struct PhysicsComponent : BaseComponent {
        uint16_t mass = 100;
        FPVector velocity;

        ~PhysicsComponent() override {}
        
        void CalculateCRC32(uint32_t& resultThusFar) override {
            resultThusFar = CRC::Calculate(&mass, sizeof(mass), CRC::CRC_32(), resultThusFar);
            velocity.CalculateCRC32(resultThusFar);
        }
    };

    struct DynamicColliderComponent : BaseComponent {
        Collider collider;

        ~DynamicColliderComponent() override {}
        
        void CalculateCRC32(uint32_t& resultThusFar) override {
            collider.CalculateCRC32(resultThusFar);
        }
    };

    struct StaticColliderComponent : BaseComponent {
        Collider collider;

        ~StaticColliderComponent() override {}
        
        void CalculateCRC32(uint32_t& resultThusFar) override {
            collider.CalculateCRC32(resultThusFar);
        }
    };

    struct HitfreezeComponent : BaseComponent {
        uint32_t startingFrame = 0;
        uint32_t totalLength = 15;

        ~HitfreezeComponent() override {}
        
        void CalculateCRC32(uint32_t& resultThusFar) override {
            resultThusFar = CRC::Calculate(&startingFrame, sizeof(startingFrame), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&totalLength, sizeof(totalLength), CRC::CRC_32(), resultThusFar);
        }
    };
}
