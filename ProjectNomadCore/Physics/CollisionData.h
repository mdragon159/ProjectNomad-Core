#pragma once

#include <EnTT/entt.hpp>
#include "Math/FPVector.h"

namespace ProjectNomad {
    class CollisionResultWithHitEntity {
    public:
        bool isColliding;
        entt::entity hitEntity = entt::null;

        CollisionResultWithHitEntity(bool inIsColliding) : isColliding(inIsColliding) {}

        CollisionResultWithHitEntity(bool inIsColliding, entt::entity inHitEntity)
            : isColliding(inIsColliding), hitEntity(inHitEntity) {}
    };

    struct ImpactResult {
        bool isColliding;

        // Axis of penetration pointing in direction from collider "A" towards collider "B"
        FPVector penetrationDirection = FPVector::zero();
        // How far collider "A" is penetrating into collider "B" along the penetration direction. Should NOT be negative
        fp penetrationMagnitude = fp{0};

        // Some well meaning but not currently used parameters below inspired by another physics (UE-PhysX code?)
        // FPVector movingColliderCenterLocationAtImpact;
        // FPVector impactLocation;
        // FPVector impactNormalAlongMovingCollider;
        // FPVector impactNormalAlongStaticCollider;
        
        ImpactResult(FPVector penDir, fp penMagnitude)
        : isColliding(true), penetrationDirection(penDir), penetrationMagnitude(penMagnitude) {}

        static ImpactResult noCollision() {
            return ImpactResult(false);
        }

    private:
        // Private as want to be careful and assure no accidental usage of this constructor occurs
        ImpactResult(bool isColliding) : isColliding(isColliding) {}
    };

    struct ImpactResultWithHitEntity {
        ImpactResult impactResult;
        entt::entity hitEntity = entt::null;

        ImpactResultWithHitEntity(ImpactResult impactResult, entt::entity hitEntity)
            : impactResult(impactResult), hitEntity(hitEntity) {}
    };
    
    class HitResultOld {
    public:
        bool isColliding;
        FPVector penetrationDepth;

        HitResultOld(bool inIsColliding) : isColliding(inIsColliding) {}

        HitResultOld(bool inIsColliding, FPVector inPenetrationDepth)
            : isColliding(inIsColliding), penetrationDepth(inPenetrationDepth) {}
    };
}
