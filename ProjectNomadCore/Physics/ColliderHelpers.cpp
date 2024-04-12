#include "ColliderHelpers.h"

#include "Model/FCollider.h"
#include "Model/Ray.h"
#include "SimpleCollisions.h"
#include "Context/CoreContext.h"
#include "Math/FQuatFP.h"

namespace ProjectNomad {
    FVectorFP ColliderHelpers::GetFurthestPoint(CoreContext& coreContext,
                                                const FCollider& collider,
                                                const FVectorFP& direction) {
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
                Ray ray(collider.GetCenter(), direction);
                fp intersectionTime;
                FVectorFP intersectionPoint;
                bool didCollide = SimpleCollisions::RaycastWithBox(
                    coreContext,
                    ray,
                    collider,
                    intersectionTime,
                    intersectionPoint
                );

                // Sanity check: Raycast should collide with box since it starts inside the box
                if (!didCollide) {
                    coreContext.logger.LogWarnMessage("Raycast from inside box somehow did not collide!");
                    return FVectorFP::Zero();
                }

                // Finally return the ray-box intersection
                return intersectionPoint;
            }

            case ColliderType::Capsule: {
                // Based on the following: https://github.com/kevinmoran/GJK/blob/master/Collider.h#L60-L67

                // Convert direction to local space as makes computing point very simple
                FVectorFP localSpaceDir = collider.ToLocalSpaceForOriginCenteredValue(direction);

                // Compute medial line length to know how far to displace the computed point
                // TODO: Why are we simply calculating like a sphere then displacing to either end of medial line?
                fp medialLineHalfLength = collider.GetMedialHalfLineLength();
                fp verticalDisplacement = localSpaceDir.z > fp{0} ? medialLineHalfLength : -medialLineHalfLength;

                // Get point along capsule's surface in requested direction
                // TODO: I don't understand why we're doing this, albeit multiple places compute it like this... .-.
                // My *guess* is that GJK/EPA don't care about the mid points along walls of capsule... for some reason?
                FVectorFP result = collider.GetCapsuleRadius() * localSpaceDir;
                result.z += verticalDisplacement;

                // Convert result to world space
                return collider.ToWorldSpaceFromLocal(result);

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
                    coreContext.logger.logWarnMessage(
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
                return collider.GetCenter() + collider.GetSphereRadius() * direction;
            }

            case ColliderType::NotInitialized:
                coreContext.logger.LogWarnMessage("Collider not initialized!");
                return FVectorFP::Zero();

            default:
                coreContext.logger.LogWarnMessage("Unknown collider type, please implement");
                return FVectorFP::Zero();
        }
    }
}
