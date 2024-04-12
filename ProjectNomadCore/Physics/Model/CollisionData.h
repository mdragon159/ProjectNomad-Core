#pragma once

#include <EnTT/entt.hpp>
#include "Math/FVectorFP.h"

namespace ProjectNomad {
    struct CollisionResultWithHitEntity {
        bool isColliding = false;
        bool didHitDynamicEntity = false;
        entt::entity hitEntity = entt::null;

        static constexpr CollisionResultWithHitEntity NoCollision() {
            CollisionResultWithHitEntity result = {};
            result.isColliding = false; // Redundant atm but nice to be explicit and robust
            return result;
        }

        static constexpr CollisionResultWithHitEntity WithCollision(entt::entity inHitEntity, bool inDidHitDynamicEntity) {
            CollisionResultWithHitEntity result = {};

            result.isColliding = true;
            result.hitEntity = inHitEntity;
            result.didHitDynamicEntity = inDidHitDynamicEntity;
            
            return result;
        }
    };

    struct ImpactResult {
        bool isColliding;

        // Axis of penetration pointing in direction from collider "A" towards collider "B"
        FVectorFP penetrationDirection = FVectorFP::Zero();
        // How far collider "A" is penetrating into collider "B" along the penetration direction. Should NOT be negative
        fp penetrationMagnitude = fp{0};

        // Some well meaning but not currently used parameters below inspired by another physics (UE-PhysX code?)
        // FPVector movingColliderCenterLocationAtImpact;
        // FPVector impactLocation;
        // FPVector impactNormalAlongMovingCollider;
        // FPVector impactNormalAlongStaticCollider;
        
        ImpactResult(FVectorFP penDir, fp penMagnitude)
        : isColliding(true), penetrationDirection(penDir), penetrationMagnitude(penMagnitude) {}

        // Copy and flip the penetration direction of this object.
        // This serves to represent the impact info penetration from the "other" object's perspective
        ImpactResult Flipped() const {
            ImpactResult result = *this;
            result.penetrationDirection.Flip();
            return result;
        }
        
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
        FVectorFP penetrationDepth;

        HitResultOld(bool inIsColliding) : isColliding(inIsColliding) {}

        HitResultOld(bool inIsColliding, FVectorFP inPenetrationDepth)
            : isColliding(inIsColliding), penetrationDepth(inPenetrationDepth) {}
    };
}
