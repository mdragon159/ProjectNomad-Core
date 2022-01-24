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
        FPVector movingColliderCenterLocationAtImpact;
        FPVector impactLocation;
        FPVector impactNormalAlongMovingCollider;
        FPVector impactNormalAlongStaticCollider;
        FPVector penetrationDepth; // Needs to be further fleshed out! May not even use in future! Heck, kinda useless looking at other properties...

        // Temp constructor for quick refactoring atm! Will also allow quick lookup for replacement
        ImpactResult(FPVector penetrationDepth) : isColliding(true), penetrationDepth(penetrationDepth) {}

        // Intended constructor for collision scenarios
        // Note that penetration depth is not part of this as don't believe it's currently necessary with other provided data
        ImpactResult(FPVector movingColliderCenterLocationAtImpact, FPVector impactLocation,
            FPVector impactNormalAlongMovingCollider, FPVector impactNormalAlongStaticCollider)
                : isColliding(true), movingColliderCenterLocationAtImpact(movingColliderCenterLocationAtImpact),
                    impactLocation(impactLocation), impactNormalAlongMovingCollider(impactNormalAlongMovingCollider),
                    impactNormalAlongStaticCollider(impactNormalAlongStaticCollider) {}

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
