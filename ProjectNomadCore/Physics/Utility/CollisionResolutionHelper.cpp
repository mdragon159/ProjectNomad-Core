#include "CollisionResolutionHelper.h"

#include "Physics/Model/CollisionData.h"
#include "Physics/Model/FCollider.h"

namespace ProjectNomad {
    void CollisionResolutionHelper::ResolveCollision(const ImpactResult& collisionResult,
                                                     const FCollider& collisionCausingCollider,
                                                     const FVectorFP& collisionCausingVelocity,
                                                     FVectorFP& postCollisionPosition,
                                                     FVectorFP& postCollisionVelocity) {
        // Assume caller of this simpler method wants all velocity in collision direction to be removed
        static constexpr fp kFullCollisionDirVelocityRemoval = fp(1);
        
        ResolveCollision(
            collisionResult.penetrationDirection,
            collisionResult.penetrationMagnitude,
            kFullCollisionDirVelocityRemoval,
            collisionCausingCollider,
            collisionCausingVelocity,
            postCollisionPosition,
            postCollisionVelocity
        );
    }

    void CollisionResolutionHelper::ResolveCollision(const FVectorFP& penetrationDirection,
                                                     fp penetrationMagnitude,
                                                     fp collisionDirVelocityReductionPercentage,
                                                     const FCollider& collisionCausingCollider,
                                                     const FVectorFP& collisionCausingVelocity,
                                                     FVectorFP& postCollisionPosition,
                                                     FVectorFP& postCollisionVelocity) {
        // Penetration dir x depth represents how much collider is penetrating in collision,
        //          and opposite direction x magnitude represents needed movement to fix.
        FVectorFP penDirAndDepth = penetrationDirection * (penetrationMagnitude + kPenDepthClearingSpace);

        // Adjust position based on calculated penetration. Note that *subtracting* penetration to get rid of it.
        // TODO: Current resolution method doesn't work well if tunneling mostly through an object already. But is that actually a concern?
        postCollisionPosition = collisionCausingCollider.GetCenter() - penDirAndDepth;

        // Remove velocity in penetration direction if necessary
        fp velocityMagnitudeInPenDirection = collisionCausingVelocity.Dot(penetrationDirection);
        if (velocityMagnitudeInPenDirection > fp{0}) {
            // Zero out the velocity that's causing the collision
            FVectorFP velocityParallelToPenetration = velocityMagnitudeInPenDirection * penetrationDirection;
            // Perpendicular = Vector - Parallel. So remove the relevant portion of the velocity only.
            postCollisionVelocity = collisionCausingVelocity - velocityParallelToPenetration * collisionDirVelocityReductionPercentage;
        }
        else {
            postCollisionVelocity = collisionCausingVelocity; // No change in velocity
        }
    }
}
