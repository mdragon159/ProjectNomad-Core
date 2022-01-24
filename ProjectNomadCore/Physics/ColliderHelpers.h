#pragma once

#include "Collider.h"
#include "Ray.h"
#include "SimpleCollisions.h"
#include "Math/FPQuat.h"
#include "Physics/Line.h"

namespace ProjectNomad {

    // Contains helper functions that SHOULD be contained within Collider but can't due to circular dependencies
    template <typename LoggerType>
    class ColliderHelpers {
        static_assert(std::is_base_of_v<ILogger, LoggerType>, "LoggerType must inherit from ILogger");
        
        LoggerType& logger;
        SimpleCollisions<LoggerType>& simpleCollisions;
    
    public:
        ColliderHelpers(LoggerType& logger, SimpleCollisions<LoggerType>& simpleCollisions)
        : logger(logger), simpleCollisions(simpleCollisions) {}
        
        /// <summary>
        /// Gets furthest point for the current collider in the given direction
        /// Used for determining support points in GJK/EPA algorithms
        /// Watch the following for a quick overview on supporting points for simplex: https://youtu.be/MDusDn8oTSE?t=80
        /// </summary>
        FPVector getFurthestPoint(const Collider& collider, const FPVector& direction) {
            switch (collider.colliderType) {
                case ColliderType::Box: {
                    /*// Based on the following: https://github.com/kevinmoran/GJK/blob/master/Collider.h#L20-L29

                    // Convert direction to local space as makes computing point very simple
                    FPVector localSpaceDir = collider.toLocalSpaceForOriginCenteredValue(direction);

                    // Compute min/max points along each axis in local space for easy logic
                    // May not be the most efficient thing in the world, but eh it's readable
                    FPVector halfSize = collider.getBoxHalfSize();
                    FPVector min = -halfSize; // Local space min points
                    FPVector max = halfSize; // Local space max points

                    // Compute furthest vertex along direction in local space
                    // FUTURE: Why vertex and not face point? Not sure, but this is what everyone is doing for GJK... >.>
                    FPVector result;
                    result.x = localSpaceDir.x > fp{0} ? max.x : min.x;
                    result.y = localSpaceDir.y > fp{0} ? max.y : min.y;
                    result.z = localSpaceDir.z > fp{0} ? max.z : min.z;

                    // Convert result to world space
                    return collider.toWorldSpaceFromLocal(result);*/

                    // Simply just a raycast from center of box to wherever it hits
                    //   Note that there ARE algorithms that try to simply return closest vertex, but personally found that
                    //   to be inaccurate and ineffective. ie, GJK SHOULD find the furthest point on surface of shape regardless
                    //   if vertex or edge or face
                    Ray ray(collider.getCenter(), direction);
                    fp intersectionTime;
                    FPVector intersectionPoint;
                    bool didCollide = simpleCollisions.raycastWithBox(
                        ray,
                        collider,
                        intersectionTime,
                        intersectionPoint
                    );

                    // Sanity check: Raycast should collide with box since it starts inside the box
                    if (!didCollide) {
                        logger.logWarnMessage(
                            "Collider::getFurthestPoint",
                            "Raycast from inside box somehow did not collide!"
                        );
                        return FPVector::zero();
                    }

                    // Finally return the ray-box intersection
                    return intersectionPoint;
                }

                case ColliderType::Capsule: {
                    // Based on the following: https://github.com/kevinmoran/GJK/blob/master/Collider.h#L60-L67

                    // Convert direction to local space as makes computing point very simple
                    FPVector localSpaceDir = collider.toLocalSpaceForOriginCenteredValue(direction);

                    // Compute medial line length to know how far to displace the computed point
                    // TODO: Why are we simply calculating like a sphere then displacing to either end of medial line?
                    fp medialLineHalfLength = collider.getMedialHalfLineLength();
                    fp verticalDisplacement = localSpaceDir.z > fp{0} ? medialLineHalfLength : -medialLineHalfLength;

                    // Get point along capsule's surface in requested direction
                    // TODO: I don't understand why we're doing this, albeit multiple places compute it like this... .-.
                    // My *guess* is that GJK/EPA don't care about the mid points along walls of capsule... for some reason?
                    FPVector result = collider.getCapsuleRadius() * localSpaceDir;
                    result.z += verticalDisplacement;

                    // Convert result to world space
                    return collider.toWorldSpaceFromLocal(result);

                    /*// FUTURE: Switch to a more efficient algorithm as can likely simplify compared to full linetest/raycast checks
    	            
    	            // Simply find intersection between ray from center of capsule to the capsule's surface
    	            //      (and yes, there are examples which simply find the point on the two hemispheres of the capsule,
    	            //          which didn't work for me and imo makes no sense in the context of capsules)
    	            // TODO: Switch to actual raycast once that's implemented. (Using linetest as that's what's implemented atm)
    	            FPVector pointFromCenterToBeyondCapsuleInDesiredDirection =
    	                collider.getCenter() + direction * (collider.getCapsuleHalfHeight() + fp{0.01f});
                    Line testLine(collider.getCenter(), pointFromCenterToBeyondCapsuleInDesiredDirection);
    	            fp intersectionTime;
    	            FPVector intersectionPoint;
    	            bool didCollide = physicsManager.linetestWithCapsule(
                        Singleton<SimContextSingleton>::get(),
                        testLine,
                        collider,
                        intersectionTime,
                        intersectionPoint
                    );

    	            // Sanity check: Line should collide with capsule since it starts inside the capsule and is longer than it
    	            if (!didCollide) {
    	                logger.logWarnMessage(
                        "Collider::getFurthestPoint",
                        "Linetest from inside capsule somehow did not collide!"
                        );
    	                return FPVector::zero();
    	            }

    	            // Finally return the ray/line-capsule intersection
    	            return intersectionPoint;*/
                }

                case ColliderType::Sphere: {
                    // Simply get point on sphere's surface along given direction
                    return collider.getCenter() + collider.getSphereRadius() * direction;
                }

                case ColliderType::NotInitialized:
                    logger.logWarnMessage(
                        "Collider::getFurthestPoint",
                        "Collider not initialized!"
                    );
                    return FPVector::zero();

                default:
                    logger.logWarnMessage(
                        "Collider::getFurthestPoint",
                        "Unknown collider type, please implement"
                    );
                    return FPVector::zero();
            }
        }
    };
}
