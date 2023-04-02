#pragma once

#include "GameCore/CoreComponents.h"
#include "Utilities/ILogger.h"
#include "Physics/PhysicsManager.h"
#include "GameCore/CoreConstants.h"
#include "Physics/Utility/CollisionResolutionHelper.h"

namespace ProjectNomad {
    template <typename LoggerType>
    class PosUpdatingAndCollisionResolutionSystem {
        static_assert(std::is_base_of_v<ILogger, LoggerType>, "LoggerType must inherit from ILogger");
        
        static constexpr uint8_t MAX_COLLISION_RESOLUTIONS_PER_FRAME = 5; // Cheap workaround for broken resolution cases
        
        LoggerType& logger;
        PhysicsManager<LoggerType>& physicsManager;
        CollisionResolutionHelper<LoggerType> collisionResolutionHelper;
        
    public:
        PosUpdatingAndCollisionResolutionSystem(LoggerType& logger, PhysicsManager<LoggerType>& physicsManager)
        : logger(logger), physicsManager(physicsManager), collisionResolutionHelper(logger) {}
        
        void Update(CoreContext& coreContext) {
            // Iterate over all (currently) physics supported actors...
            auto view = coreContext.registry.view<TransformComponent, PhysicsComponent, DynamicColliderComponent>();
            for (auto &&[entityId, transform, physicsComp, colliderComp] : view.each()) {
                // Calculate new position then check for collisions with that new position
                FPVector newIntendedPos = calculateNewPositionFromVelocity(coreContext, entityId, transform, physicsComp);
                FPVector correctedPos =
                    assureNoCollisionsWithNewPos(coreContext, entityId, colliderComp.collider, physicsComp, newIntendedPos);

                // Set the post-collision check position
                setEntityPosition(transform, colliderComp.collider, correctedPos);
            }
        }

    private:
        static FPVector calculateNewPositionFromVelocity(CoreContext& coreContext,
                                                        const entt::entity& entityId,
                                                        const TransformComponent& transform,
                                                        const PhysicsComponent& physicsComp) {

            if (!coreContext.registry.any_of<HitstopComponent>(entityId)) { // Preferably this system wouldn't need to care about hitfreeze for performance, but quick and dirty way to prevent movement during hitfreeze
                const FPVector& playerVel = physicsComp.velocity;
                fp changeInXPos = playerVel.x * CoreConstants::GetTimePerFrameInSec();
                fp changeInYPos = playerVel.y * CoreConstants::GetTimePerFrameInSec();
                fp changeInZPos = playerVel.z * CoreConstants::GetTimePerFrameInSec();

                const FPVector& playerLocation = transform.location;
                return {
                    playerLocation.x + changeInXPos,
                    playerLocation.y + changeInYPos,
                    playerLocation.z + changeInZPos
                  };
            }

            return transform.location; // If in hitfreeze, don't move player at all
        }

        FPVector assureNoCollisionsWithNewPos(CoreContext& coreContext,
                                             const entt::entity& selfId,
                                             const Collider& collider,
                                             PhysicsComponent& physicsComp,
                                             FPVector newIntendedPos) {

            // Intention: Collision resolution with one object may result in colliding with another object
            // Thus, keep retrying collision resolution until no collision (or until hit limit)
            uint8_t totalCollisionsSoFar = 0;
            while(true) { // TODO: Determine if this is actually important AND useful. (Eg, actually works well in cases where colliding with multiple objects)
                bool wasCollisionFound = checkForAndResolveCollisions(coreContext, selfId, collider, physicsComp, newIntendedPos);
                if (!wasCollisionFound) {
                    break;
                }
                
                totalCollisionsSoFar++;
                if (totalCollisionsSoFar >= MAX_COLLISION_RESOLUTIONS_PER_FRAME) {
                    // logger.addScreenAndLogMessage(fp{2}, "WARNING: Hit max collisions per frame");
                    break;
                }
            }

            return newIntendedPos;
        }

        bool checkForAndResolveCollisions(CoreContext& coreContext,
                                        const entt::entity& selfId,
                                        const Collider& currentCollider,
                                        PhysicsComponent& physicsComp,
                                        FPVector& newIntendedPos) {

            // CapsuleOld futureBoundingShape = collider.capsule;
            // futureBoundingShape.center = newIntendedPos;
            Collider futureBoundingShape(currentCollider);
            futureBoundingShape.setCenter(newIntendedPos);

            bool wasCollisionEverFound = false;
            
            // Check if new desired position is colliding with any static objects
            auto staticView = coreContext.registry.view<StaticColliderComponent>();
            for (auto &&[entityId, staticColliderComp] : staticView.each()) {
                bool collisionFound =
                    checkAndResolveIndividualCollision(futureBoundingShape, staticColliderComp.collider, physicsComp);
                if (collisionFound) {
                    wasCollisionEverFound = true;
                }
            }

            // Check if new desired position is colliding with any dynamic objects
            auto dynamicView = coreContext.registry.view<DynamicColliderComponent>();
            for (auto &&[entityId, otherColliderComp] : dynamicView.each()) {
                if (entityId == selfId) {
                    continue;
                }
                
                bool collisionFound =
                    checkAndResolveIndividualCollision(futureBoundingShape, otherColliderComp.collider,physicsComp);
                if (collisionFound) {
                    wasCollisionEverFound = true;
                }
            }

            newIntendedPos = futureBoundingShape.center;
            return wasCollisionEverFound;
        }

        bool checkAndResolveIndividualCollision(Collider& futureCollider,
                                                const Collider& checkAgainstCollider,
                                                PhysicsComponent& physicsComp) {
            
            ImpactResult collisionResult = physicsManager.getComplexCollisions().isColliding(futureCollider, checkAgainstCollider);
            if (collisionResult.isColliding) {
                FPVector postCollisionPosition;
                FPVector postCollisionVelocity;
                collisionResolutionHelper.resolveCollisionTest(
                    collisionResult, futureCollider, physicsComp.velocity,
                    postCollisionPosition, postCollisionVelocity
                );

                futureCollider.center = postCollisionPosition;
                physicsComp.velocity = postCollisionVelocity;

                return true;
            }

            return false;
        }

        static void setEntityPosition(TransformComponent& transform, Collider& collider, const FPVector& newPos) {
            transform.location = newPos;
            collider.setCenter(newPos);
        }
    };
}
