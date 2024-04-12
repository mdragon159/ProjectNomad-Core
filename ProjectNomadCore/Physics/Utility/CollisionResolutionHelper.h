#pragma once

#include "fpm/FixedPoint.h"
#include "Math/FixedPoint.h"

struct FVectorFP;
struct FCollider;

namespace ProjectNomad {
    struct ImpactResult;

    class CollisionResolutionHelper {
      public:
        CollisionResolutionHelper() = delete;

        static void ResolveCollision(const ImpactResult& collisionResult,
                                     const FCollider& collisionCausingCollider,
                                     const FVectorFP& collisionCausingVelocity,
                                     FVectorFP& postCollisionPosition,
                                     FVectorFP& postCollisionVelocity);
        static void ResolveCollision(const FVectorFP& penetrationDirection,
                                     fp penetrationMagnitude,
                                     fp collisionDirVelocityReductionPercentage,
                                     const FCollider& collisionCausingCollider,
                                     const FVectorFP& collisionCausingVelocity,
                                     FVectorFP& postCollisionPosition,
                                     FVectorFP& postCollisionVelocity);

      private:
        // Use a *small* amount of space to assure the collision resolution clears any possible tiiiiny overlap.
        //      FUTURE: This is really just a band-aid. Would be nice to fix the root cause so this isn't necessary
        //      Sample cases: Standing capsule vs sphere (mostly horizontal). Or sphere in the lower edge of a tilted box
        //      TODO: The rotated box vs sphere seems to be especially bad. Box vs sphere logic needs to be revisited
        static constexpr FFixedPoint kPenDepthClearingSpace = FFixedPoint(0.25f); // Low value seems to work pretty well for simple cases, and multiple passes helps with pretty bad cases
    };
}
