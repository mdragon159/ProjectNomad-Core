#include "HandleDynamicVsStaticCollisions.h"

#include "Context/SimContext.h"
#include "GameCore/CoreComponents.h"
#include "Helpers/PhysicsUpdateHelpers.h"
#include "Physics/ComplexCollisions.h"
#include "Physics/Model/CollisionData.h"
#include "Physics/Model/FCollider.h"
#include "Physics/Utility/CollisionResolutionHelper.h"
#include "Utilities/Profiling.h"

namespace ProjectNomad {
    void HandleDynamicVsStaticCollisions::ProcessStaticCollisionsUntilLimitReached(
                                                                                SimContext& simContext,
                                                                                entt::entity selfId,
                                                                                const DynamicColliderComponent& colliderComp,
                                                                                PhysicsComponent& physicsComp,
                                                                                FVectorFP& newIntendedPos,
                                                                                bool& wasMaxCollisionPassesReached) {
        const auto& gameplayConstants = simContext.GetStaticGameplayData().gameplayConstants; // For readability

        // Intention: Collision resolution with one object may result in colliding with another object
        // Thus, keep retrying collision resolution until no collision (or until hit limit).
        uint8_t totalCollisionPasses = 0;
        while (totalCollisionPasses < gameplayConstants.maxCollisionResolutionsPerFrame) {
            bool wasCollisionFound = DoSinglePassCollisionCheckingAndResolution(
                simContext, selfId, colliderComp, physicsComp, newIntendedPos
            );
            if (!wasCollisionFound) {
                break;
            }

            totalCollisionPasses++;
        }

        wasMaxCollisionPassesReached = totalCollisionPasses >= gameplayConstants.maxCollisionResolutionsPerFrame;
    }
    
    void HandleDynamicVsStaticCollisions::Update(SimContext& simContext) {
        MEASURE_SYSTEM_FUNCTION("HandleDynamicVsStaticCollisions", STAT_SYSTEM_HandleDynamicVsStaticCollisions);

        auto view = simContext.registry.view<PhysicsComponent, TransformComponent, DynamicColliderComponent>();
        for (auto&& [entityId, physicsComp, transformComp, colliderComp] : view.each()) {
            OnUpdate(simContext, entityId, physicsComp, transformComp, colliderComp);
        }
    }

    void HandleDynamicVsStaticCollisions::OnUpdate(SimContext& simContext,
                                                   entt::entity selfId,
                                                   PhysicsComponent& physicsComp,
                                                   TransformComponent& transformComp,
                                                   DynamicColliderComponent& colliderComp) {
        // Start off with the entity's current location, as that's where they "want" to be atm
        FVectorFP intendedPos = transformComp.location;

        // Check for and handle any collisions based on the intended position.
        //      Note that if collisions occur, then the "intended" position is expected to change accordingly.
        bool wasCollisionPassLimitReached = false;
        ProcessStaticCollisionsUntilLimitReached(
            simContext, selfId, colliderComp, physicsComp, intendedPos, wasCollisionPassLimitReached
        );

        // Hitting the max limit of collision passes should *not* be the norm
        if (UNLIKELY(wasCollisionPassLimitReached)) {
            simContext.logger.LogWarnMessage("FYI: Hit max static collisions per frame");
        }

        // Finally set the resultant location.
        //      We *could* only set the resulting location if there were actually any collisions, but not really saving
        //      much in terms of performance. Might as well keep it simpler including the method parameters
        PhysicsUpdateHelpers::SetNewLocation(transformComp, colliderComp, intendedPos);
    }

    bool HandleDynamicVsStaticCollisions::DoSinglePassCollisionCheckingAndResolution(
                                                                        SimContext& simContext,
                                                                        const entt::entity& selfId,
                                                                        const DynamicColliderComponent& colliderComp,
                                                                        PhysicsComponent& physicsComp,
                                                                        FVectorFP& newIntendedPos) {
        FCollider futureBoundingShape = colliderComp.collider.CopyWithNewCenter(newIntendedPos);
        bool wasCollisionEverFound = false;

        // Check if new desired position is colliding with any static objects
        auto view = simContext.registry.view<StaticColliderComponent>();
        for (auto&& [entityId, colliderComp] : view.each()) {
            bool collisionFound = CheckAndResolveIndividualCollision(
                simContext, futureBoundingShape, colliderComp.collider, physicsComp
            );
            
            if (collisionFound) {
                wasCollisionEverFound = true;
            }
        }

        newIntendedPos = futureBoundingShape.center;
        return wasCollisionEverFound;
    }

    bool HandleDynamicVsStaticCollisions::CheckAndResolveIndividualCollision(SimContext& simContext,
                                                                             FCollider& futureCollider,
                                                                             const FCollider& checkAgainstCollider,
                                                                             PhysicsComponent& physicsComp) {
        ImpactResult collisionResult = ComplexCollisions::IsColliding(simContext, futureCollider, checkAgainstCollider);
        
        if (collisionResult.isColliding) {
            FVectorFP postCollisionPosition;
            FVectorFP postCollisionVelocity;
            CollisionResolutionHelper::ResolveCollision(
                collisionResult, futureCollider, physicsComp.velocity,
                postCollisionPosition, postCollisionVelocity
            );

            futureCollider.center = postCollisionPosition;
            physicsComp.velocity = postCollisionVelocity;
            return true;
        }

        return false;
    }
}
