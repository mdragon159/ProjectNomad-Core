#pragma once

#include "GameCore/BaseSystem.h"
#include "GameCore/CoreComponents.h"
#include "Utilities/ILogger.h"
#include "Physics/PhysicsManager.h"
#include "GameCore/CoreConstants.h"

namespace ProjectNomad {
    template <typename LoggerType>
    class PosUpdatingAndCollisionResolutionSystem : public BaseSystem {
        static_assert(std::is_base_of_v<ILogger, LoggerType>, "LoggerType must inherit from ILogger");
        
        static constexpr uint8_t MAX_COLLISION_RESOLUTIONS_PER_FRAME = 5; // Cheap workaround for broken resolution cases
        
        LoggerType& logger;
        PhysicsManager<LoggerType>& physicsManager;
        
    public:
        PosUpdatingAndCollisionResolutionSystem(LoggerType& logger, PhysicsManager<LoggerType>& physicsManager)
        : logger(logger), physicsManager(physicsManager) {}
        ~PosUpdatingAndCollisionResolutionSystem() override {}
        
        void update(entt::registry& registry) override {
            // Iterate over all (currently) physics supported actors...
            auto view = registry.view<TransformComponent, PhysicsComponent, DynamicColliderComponent>();
            for (auto &&[entityId, transform, physicsComp, colliderComp] : view.each()) {
                // Calculate new position then check for collisions with that new position
                FPVector newIntendedPos = calculateNewPositionFromVelocity(registry, entityId, transform, physicsComp);
                FPVector correctedPos =
                    assureNoCollisionsWithNewPos(registry, entityId, colliderComp.collider, physicsComp, newIntendedPos);

                // Set the post-collision check position
                setEntityPosition(transform, colliderComp.collider, correctedPos);
            }
        }

    private:
        static FPVector calculateNewPositionFromVelocity(entt::registry& registry,
                                                        const entt::entity& entityId,
                                                        const TransformComponent& transform,
                                                        const PhysicsComponent& physicsComp) {

            if (!registry.any_of<HitfreezeComponent>(entityId)) { // Preferably this system wouldn't need to care about hitfreeze for performance, but quick and dirty way to prevent movement during hitfreeze
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

        FPVector assureNoCollisionsWithNewPos(entt::registry& registry,
                                             const entt::entity& selfId,
                                             const Collider& collider,
                                             PhysicsComponent& physicsComp,
                                             FPVector newIntendedPos) {

            // Intention: Collision resolution with one object may result in colliding with another object
            // Thus, keep retrying collision resolution until no collision (or until hit limit)
            uint8_t totalCollisionsSoFar = 0;
            while(true) {
                bool wasCollisionFound = checkForAndResolveCollisions(registry, selfId, collider, physicsComp, newIntendedPos);
                if (!wasCollisionFound) {
                    break;
                }
                
                totalCollisionsSoFar++;
                if (totalCollisionsSoFar >= MAX_COLLISION_RESOLUTIONS_PER_FRAME) {
                    // mSimContext.addScreenAndLogMessage(2.f, "WARNING: Hit max collisions per frame");
                    break;
                }
            }

            return newIntendedPos;
        }

        bool checkForAndResolveCollisions(entt::registry& registry,
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
            auto staticView = registry.view<StaticColliderComponent>();
            for (auto &&[entityId, staticColliderComp] : staticView.each()) {
                bool collisionFound =
                    checkAndResolveIndividualCollision(futureBoundingShape, staticColliderComp.collider, physicsComp, newIntendedPos);
                if (collisionFound) {
                    wasCollisionEverFound = true;
                }
            }

            // Check if new desired position is colliding with any dynamic objects
            auto dynamicView = registry.view<DynamicColliderComponent>();
            for (auto &&[entityId, otherColliderComp] : dynamicView.each()) {
                if (entityId == selfId) {
                    continue;
                }
                
                bool collisionFound =
                    checkAndResolveIndividualCollision(futureBoundingShape, otherColliderComp.collider,
                        physicsComp, newIntendedPos);
                if (collisionFound) {
                    wasCollisionEverFound = true;
                }
            }

            return wasCollisionEverFound;
        }

        bool checkAndResolveIndividualCollision(Collider& futureCollider,
                                                const Collider& checkAgainstCollider,
                                                PhysicsComponent& physicsComp,
                                                FPVector& newIntendedPos) {

            // TODO: Rewrite this entire cursed function once GJK/EPA/etc algorithms with additional resolution data are ready
            
            ImpactResult collisionResult = physicsManager.getComplexCollisions()
                .isColliding(futureCollider, checkAgainstCollider);
            if (collisionResult.isColliding) {               
                // mSimContext->getSharedData()->mSimData.mDebugMessages.push(
                //   DebugMessage::createTextMessage(
                //     0.2f, OutputLocation::Screen, "Is colliding! Pen depth: " + collisionResult.mPenetrationDepth.toString()
                //   )
                // );

                // Penetration depth is a simple displacement value. Need to determine whether to correct towards or away
                // TODO: Current resolution method doesn't work well if tunneling mostly through an object already
                //        In reality, current resolution would teleport you on other side of object instead of where you came from
                //        Need to fix... sometime in future. Current idea is to check whether pen depth would put you on other side of object,
                //        but that also has its fun edge cases- likely need to sweep for high speeds anywhos. Tradeoffs for future!
                // Normalizing for velocity correction case
                FPVector penDirAndDepth = collisionResult.penetrationDirection * collisionResult.penetrationMagnitude;
                fp velocityMagnitudeInPenDirection = physicsComp.velocity.dot(collisionResult.penetrationDirection);
                bool didVelocityCauseCollision = false;
                if (velocityMagnitudeInPenDirection > fp{0}) {
                    // Is penetration depth trying to move us in same direction as we were already going?
                    // If penetration depth is pointing us towards the collision (assuming velocity caused collision),
                    //  then subtract to get away from the collision
                    newIntendedPos -= penDirAndDepth;
                    futureCollider.center = newIntendedPos;

                    didVelocityCauseCollision = true;
                } else if (velocityMagnitudeInPenDirection < fp{0}) {
                    // Is penetration depth trying to move us opposite from where player was trying to go?
                    // Add penetration depth since that should take the player away from the collision
                    // (again, assuming velocity caused collision)
                    newIntendedPos += penDirAndDepth;
                    futureCollider.center = newIntendedPos;

                    didVelocityCauseCollision = true;
                } else {
                    // In this case, velocity and pen depth are perpendicular so can't rely on velocity to determine what to do
                    // Note that sometimes pen depth will point towards the object, sometimes away
                    // No simple pattern to rely on with underlying calculation
                    // Thus, we need some other method to determine how to apply pen depth

                    // Base rule is as follows: Add pen depth if it will not cause collision. Subtract if it will cause collision
                    // We could just guess and check (eg, add -> check if colliding again -> if so, subtract instead) but that's expensive
                    // Instead, idea is to try using where colliding object is relative to player in order to correct

                    // TODO: This assumes simple similarly sized box-to-box. Likely will work with spheres and capsules, but 
                    // be careful in future when adding other combos! 
                    // Also will need to deal with size differences for easy tunneling... eventually
                    // Likely can just use old player pos instead of new pos for direction comparison
                    // Orrr... just say screw it and guess and check

                    FPVector playerToObjectVector = checkAgainstCollider.getCenter() - newIntendedPos;
                    fp playerToObjectVsPenDepthDir = playerToObjectVector.dot(collisionResult.penetrationDirection);
                    if (playerToObjectVsPenDepthDir > fp{0}) {
                        // Pen depth pointing towards object, ie adding would not resolve collision
                        newIntendedPos -= penDirAndDepth;
                        futureCollider.center = newIntendedPos;
                    } else if (playerToObjectVsPenDepthDir < fp{0}) {
                        // Pen depth pointing away from collision object
                        newIntendedPos += penDirAndDepth;
                        futureCollider.center = newIntendedPos;
                    } else {
                        // Praying this case doesn't happen out of laziness but reality will likely differ
                        // In future, could just do the guess and check method but for now pick one + log
                        newIntendedPos += penDirAndDepth;
                        futureCollider.center = newIntendedPos;

                        // mSimContext.addDebugMessage(
                        //     DebugMessage::createTextMessage(
                        //         10, OutputLocation::LogAndScreen,
                        //         "UNEXPECTED: Fallback position collision resolution case failed!"
                        //     )
                        // );
                    }
                }

                if (didVelocityCauseCollision) {
                    // Zero out the velocity that's causing the collision
                    FPVector velocityParallelToPenetration = velocityMagnitudeInPenDirection * penetrationDirection;
                    physicsComp.velocity = physicsComp.velocity - velocityParallelToPenetration; // Perpendicular = Vector - Parallel
                }

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
