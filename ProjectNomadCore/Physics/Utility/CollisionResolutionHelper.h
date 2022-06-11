#pragma once

#include "GameCore/CoreComponents.h"
#include "Utilities/ILogger.h"
#include "GameCore/CoreConstants.h"
#include "Physics/CollisionData.h"

namespace ProjectNomad {
    template <typename LoggerType>
    class CollisionResolutionHelper {
        static_assert(std::is_base_of_v<ILogger, LoggerType>, "LoggerType must inherit from ILogger");
        
        LoggerType& logger;
        
    public:
        CollisionResolutionHelper(LoggerType& logger) : logger(logger) {}
        ~CollisionResolutionHelper() {}
        
        void resolveCollision(const ImpactResult& collisionResult,
                                const Collider& collisionCausingCollider,
                                const FPVector& collisionCausingVelocity,
                                FPVector& postCollisionPosition,
                                FPVector& postCollisionVelocity) {
            postCollisionPosition = collisionCausingCollider.getCenter(); // Calculations assume this starts at original position

            // Penetration depth is a simple displacement value. Need to determine whether to correct towards or away
            // TODO: Current resolution method doesn't work well if tunneling mostly through an object already
            //        In reality, current resolution would teleport you on other side of object instead of where you came from
            //        Need to fix... sometime in future. Current idea is to check whether pen depth would put you on other side of object,
            //        but that also has its fun edge cases- likely need to sweep for high speeds anywhos. Tradeoffs for future!
            // Normalizing for velocity correction case
            FPVector penDirAndDepth = collisionResult.penetrationDirection * collisionResult.penetrationMagnitude;
            fp velocityMagnitudeInPenDirection = collisionCausingVelocity.dot(collisionResult.penetrationDirection);
            bool didVelocityCauseCollision = false;
            if (velocityMagnitudeInPenDirection > fp{0}) {
                // Is penetration depth trying to move us in same direction as we were already going?
                // If penetration depth is pointing us towards the collision (assuming velocity caused collision),
                //  then subtract to get away from the collision
                postCollisionPosition -= penDirAndDepth;

                didVelocityCauseCollision = true;
            } else if (velocityMagnitudeInPenDirection < fp{0}) {
                // Is penetration depth trying to move us opposite from where player was trying to go?
                // Add penetration depth since that should take the player away from the collision
                // (again, assuming velocity caused collision)
                postCollisionPosition += penDirAndDepth;

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

                FPVector playerToObjectVector = collisionCausingCollider.getCenter() - postCollisionPosition;
                fp playerToObjectVsPenDepthDir = playerToObjectVector.dot(collisionResult.penetrationDirection);
                if (playerToObjectVsPenDepthDir > fp{0}) {
                    // Pen depth pointing towards object, ie adding would not resolve collision
                    postCollisionPosition -= penDirAndDepth;
                } else if (playerToObjectVsPenDepthDir < fp{0}) {
                    // Pen depth pointing away from collision object
                    postCollisionPosition += penDirAndDepth;
                } else {
                    // Praying this case doesn't happen out of laziness but reality will likely differ
                    // In future, could just do the guess and check method but for now pick one + log
                    postCollisionPosition += penDirAndDepth;

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
                FPVector velocityParallelToPenetration = velocityMagnitudeInPenDirection * collisionResult.penetrationDirection;
                postCollisionVelocity = collisionCausingVelocity - velocityParallelToPenetration; // Perpendicular = Vector - Parallel
            }
            else {
                postCollisionVelocity = collisionCausingVelocity; // No change in velocity
            }
        }
    };
}
