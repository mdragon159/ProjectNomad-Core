#include "HandleDynamicVsDynamicCollisions.h"

#include "Context/SimContext.h"
#include "GameCore/CoreComponents.h"
#include "Helpers/PhysicsUpdateHelpers.h"
#include "Physics/ComplexCollisions.h"
#include "Physics/Model/CollisionData.h"
#include "Physics/Model/FCollider.h"
#include "Physics/Utility/CollisionResolutionHelper.h"
#include "Utilities/Profiling.h"

namespace ProjectNomad {
    void HandleDynamicVsDynamicCollisions::TryMoveToLocationAndProcessDynamicCollisions(
                                                                                SimContext& simContext,
                                                                                entt::entity selfId,
                                                                                TransformComponent& transformComp,
                                                                                DynamicColliderComponent& colliderComp,
                                                                                PhysicsComponent& physicsComp,
                                                                                const FVectorFP& newIntendedPos) {
        const auto& gameplayConstants = simContext.GetStaticGameplayData().gameplayConstants; // For readability

        // Always try setting the post-collision intended position.
        //      In short: We always set the location for simplicity (given no adverse effects).
        //      Longer reasoning:
        //          At time of writing, HandleDynamicVsDynamicCollisions system doesn't *really* need to set the position
        //          if no collisions occur. However, calls from MovementHelper::TryMoveToLocation always tries to move
        //          to the new position. Given there's no harm in worst case scenario of setting an
        //          existing position again (assumption), we can always set the result position regardless of collisions.
        //      Bonus: Simplifies APIs. If we're typically immediately moving the position of other dynamic entities
        //              we collide with, then there's no real point in *not* immediately moving the position here as well.
        PhysicsUpdateHelpers::SetNewLocation(transformComp, colliderComp, newIntendedPos);

        // Intention: Collision resolution with one object may result in colliding with another object
        // Thus, keep retrying collision resolution until no collision (or until hit limit).
        uint8_t totalCollisionPasses = 0;
        while (totalCollisionPasses < gameplayConstants.maxCollisionResolutionsPerFrame) {
            bool wasCollisionFound = DoSinglePassCollisionCheckingAndResolution(
                simContext, selfId, transformComp, colliderComp, physicsComp
            );
            if (!wasCollisionFound) {
                break;
            }

            totalCollisionPasses++;
        }

        // Hitting the max limit of collision passes should *not* be the norm
        if (UNLIKELY(totalCollisionPasses >= gameplayConstants.maxCollisionResolutionsPerFrame)) {
            // Currently... hitting this edge case *is* somewhat the norm. Thus disable this warning for now so doesn't spam console
            // simContext.logger.LogWarnMessage("FYI: Hit max dynamic collisions per frame");
        }
    }
    
    void HandleDynamicVsDynamicCollisions::Update(SimContext& simContext) {
        MEASURE_SYSTEM_FUNCTION("HandleDynamicVsDynamicCollisions", STAT_SYSTEM_HandleDynamicVsDynamicCollisions);

        auto view = simContext.registry.view<PhysicsComponent, TransformComponent, DynamicColliderComponent>();
        for (auto&& [entityId, physicsComp, transformComp, colliderComp] : view.each()) {
            OnUpdate(simContext, entityId, physicsComp, transformComp, colliderComp);
        }
    }

    void HandleDynamicVsDynamicCollisions::OnUpdate(SimContext& simContext,
                                                   entt::entity selfId,
                                                   PhysicsComponent& physicsComp,
                                                   TransformComponent& transformComp,
                                                   DynamicColliderComponent& colliderComp) {
        // Start off with "moving" to the entity's current location, as that's where the entity "wants" to be atm
        FVectorFP intendedPos = transformComp.location;

        // Check for and handle any collisions based on the intended position.
        TryMoveToLocationAndProcessDynamicCollisions(
            simContext, selfId, transformComp, colliderComp, physicsComp, intendedPos
        );
    }

    bool HandleDynamicVsDynamicCollisions::DoSinglePassCollisionCheckingAndResolution(
                                                                        SimContext& simContext,
                                                                        entt::entity movingEntityId,
                                                                        TransformComponent& movingTransformComp,
                                                                        DynamicColliderComponent& movingColliderComp,
                                                                        PhysicsComponent& movingPhysicsComp) {
        bool wasCollisionEverFound = false;

        // Check if new desired position is colliding with any dynamic objects
        auto view = simContext.registry.view<DynamicColliderComponent, PhysicsComponent, TransformComponent>();
        for (auto&& [otherEntityId, otherColliderComp, otherPhysicsComp, otherTransformComp] : view.each()) {
            if (otherEntityId == movingEntityId) {
                continue;
            }
            
            bool collisionFound = CheckAndResolveIndividualCollision(
                simContext, movingTransformComp, movingColliderComp, movingPhysicsComp,
                otherTransformComp, otherColliderComp, otherPhysicsComp
            );
            
            if (collisionFound) {
                wasCollisionEverFound = true;
            }
        }

        return wasCollisionEverFound;
    }

    bool HandleDynamicVsDynamicCollisions::CheckAndResolveIndividualCollision(
                                                                        SimContext& simContext,
                                                                        TransformComponent& firstTransformComp,
                                                                        DynamicColliderComponent& firstColliderComp,
                                                                        PhysicsComponent& firstPhysicsComp,
                                                                        TransformComponent& secondTransformComp,
                                                                        DynamicColliderComponent& secondColliderComp,
                                                                        PhysicsComponent& secondPhysicsComp) {
        ImpactResult collisionResultFromFirstObjectPerspective = ComplexCollisions::IsColliding(
            simContext, firstColliderComp.collider, secondColliderComp.collider
        );

        if (!collisionResultFromFirstObjectPerspective.isColliding) {
            return false; // No collision found
        }

        // Is mass equal? Then apply the collision resolution equally between both objects
        if (firstPhysicsComp.mass == secondPhysicsComp.mass) {
            // Feels awful for player to have their velocity 100% removed when running straight into a stationary entity.
            // Thus only remove a portion of that velocity.
            fp reduceCollisionDirVelocityByHalf = fp(0.5f);

            // Apply half of penetration to both sides for equal resolution
            fp halvedPenetrationMagnitude = collisionResultFromFirstObjectPerspective.penetrationMagnitude / 2;
            
            ApplyCollisionResolution(
                collisionResultFromFirstObjectPerspective.penetrationDirection,
                halvedPenetrationMagnitude,
                reduceCollisionDirVelocityByHalf,
                firstTransformComp,
                firstColliderComp,
                firstPhysicsComp
            );
            ApplyCollisionResolution(
                collisionResultFromFirstObjectPerspective.penetrationDirection.Flipped(),
                halvedPenetrationMagnitude,
                reduceCollisionDirVelocityByHalf,
                secondTransformComp,
                secondColliderComp,
                secondPhysicsComp
            );

            return true; // Collision was found
        }

        // Otherwise mass is different and needs bit more work for resolution.
        // For ease of comparison, check which object is heavier then compute mass ratio based on that and proceed.
        if (firstPhysicsComp.mass > secondPhysicsComp.mass) {
            fp massRatioWithFirstObjectAsNumerator = firstPhysicsComp.mass / secondPhysicsComp.mass;
            ResolveCollisionBetweenDifferentWeights(
                simContext,
                massRatioWithFirstObjectAsNumerator,
                firstTransformComp,
                firstColliderComp,
                firstPhysicsComp,
                collisionResultFromFirstObjectPerspective.Flipped(),
                secondTransformComp,
                secondColliderComp,
                secondPhysicsComp
            );
        }
        else {
            fp massRatioWithSecondObjectAsNumerator = secondPhysicsComp.mass / firstPhysicsComp.mass;
            ResolveCollisionBetweenDifferentWeights(
                simContext,
                massRatioWithSecondObjectAsNumerator,
                secondTransformComp,
                secondColliderComp,
                secondPhysicsComp,
                collisionResultFromFirstObjectPerspective,
                firstTransformComp,
                firstColliderComp,
                firstPhysicsComp
            );
        }

        return true; // Collision was found
    }

    void HandleDynamicVsDynamicCollisions::ResolveCollisionBetweenDifferentWeights(
                                                        SimContext& simContext,
                                                        fp massRatioWithHeavierObjectAsNumerator,
                                                        TransformComponent& heavierObjectTransform,
                                                        DynamicColliderComponent& heavierObjectCollider,
                                                        PhysicsComponent& heavierObjectPhysics,
                                                        const ImpactResult& impactInfoFromLighterObjectPerspective,
                                                        TransformComponent& lighterObjectTransform,
                                                        DynamicColliderComponent& lighterObjectCollider,
                                                        PhysicsComponent& lighterObjectPhysics) {
        // If mass ratio greater than a certain threshold, then treat the heavier object as "too heavy" to affect.
        //      Could just not do this explicit check and allow very smol numbers, but this lets us design mass ratios
        //      that have this absolute effect without going to very smol or chongus mass values that *feel* absolute.
        if (massRatioWithHeavierObjectAsNumerator >= simContext.GetGameplayConstants().massRatioForDistributionDynamicCollisions) {
            static constexpr fp kFullCollisionDirVelocityRemoval = fp(1);
            ApplyCollisionResolution(
                impactInfoFromLighterObjectPerspective.penetrationDirection,
                impactInfoFromLighterObjectPerspective.penetrationMagnitude,
                kFullCollisionDirVelocityRemoval,
                lighterObjectTransform,
                lighterObjectCollider,
                lighterObjectPhysics
            );
            
            return;
        }

        // Otherwise use the ratio of total mass for how much of the collision resolution to apply for each object.
        fp totalMass = heavierObjectPhysics.mass + lighterObjectPhysics.mass;
        fp heavierObjectTotalMassRatio = heavierObjectPhysics.mass / totalMass;
        fp lighterObjectTotalMassRatio = 1 - heavierObjectTotalMassRatio;
        
        fp heavierObjectPenetrationMagnitude = impactInfoFromLighterObjectPerspective.penetrationMagnitude * (1 - heavierObjectTotalMassRatio);
        ApplyCollisionResolution(
            impactInfoFromLighterObjectPerspective.penetrationDirection.Flipped(),
            heavierObjectPenetrationMagnitude,
            heavierObjectTotalMassRatio,
            heavierObjectTransform,
            heavierObjectCollider,
            heavierObjectPhysics
        );

        // Use the remaining penetration depth for the lighter object collision resolution.
        //      Note that there's no intended reason why resolving one or other first. Just arbitrarily chose one earlier.
        ApplyCollisionResolution(
            impactInfoFromLighterObjectPerspective.penetrationDirection,
            impactInfoFromLighterObjectPerspective.penetrationMagnitude - heavierObjectPenetrationMagnitude,
            lighterObjectTotalMassRatio,
            lighterObjectTransform,
            lighterObjectCollider,
            lighterObjectPhysics
        );
    }

    void HandleDynamicVsDynamicCollisions::ApplyCollisionResolution(const FVectorFP& penetrationDirection,
                                                                    fp penetrationMagnitude,
                                                                    fp collisionDirVelocityReductionPercentage,
                                                                    TransformComponent& transformComp,
                                                                    DynamicColliderComponent& colliderComp,
                                                                    PhysicsComponent& physicsComp) {
        // Calculate the post-collision position and velocity
        FVectorFP postCollisionPosition;
        FVectorFP postCollisionVelocity;
        CollisionResolutionHelper::ResolveCollision(
            penetrationDirection, penetrationMagnitude, collisionDirVelocityReductionPercentage,
            colliderComp.collider, physicsComp.velocity,
            postCollisionPosition, postCollisionVelocity
        );

        // Apply the new location and velocity
        PhysicsUpdateHelpers::SetNewLocation(transformComp, colliderComp, postCollisionPosition);
        physicsComp.velocity = postCollisionVelocity;
    }
}
