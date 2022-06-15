#pragma once

#include "Math/FixedPoint.h"
#include "Math/FPMath.h"
#include "Math/FPVector.h"
#include "CollisionData.h"
#include "CollisionHelpers.h"
#include "Line.h"
#include "Ray.h"
#include "SimpleCollisions.h"
#include "Collider.h"
#include "ColliderHelpers.h"
#include "Simplex.h"
#include "Math/VectorUtilities.h"

namespace ProjectNomad {
    template <typename LoggerType>
    class ComplexCollisions {
        static_assert(std::is_base_of_v<ILogger, LoggerType>, "LoggerType must inherit from ILogger");
        
        LoggerType& logger;
        SimpleCollisions<LoggerType>& simpleCollisions;
        ColliderHelpers<LoggerType> colliderHelpers;
    
    public:
        ComplexCollisions(LoggerType& logger, SimpleCollisions<LoggerType>& simpleCollisions)
        : logger(logger), simpleCollisions(simpleCollisions), colliderHelpers(logger, simpleCollisions) {} 

#pragma region Get Hit Info Complex Collision Checks
        ImpactResult isColliding(const Collider& A, const Collider& B) {
            if (A.isNotInitialized()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isColliding",
                    "Collider A was not initialized type"
                );
                return ImpactResult::noCollision();
            }
            if (B.isNotInitialized()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isColliding",
                    "Collider B was not initialized type"
                );
                return ImpactResult::noCollision();
            }

            if (A.isBox()) {
                if (B.isBox()) {
                    return isBoxAndBoxColliding(A, B);
                }
                if (B.isCapsule()) {
                    return isBoxAndCapsuleColliding(A, B);
                }
                if (B.isSphere()) {
                    return isBoxAndSphereColliding(A, B);
                }
            }
            if (A.isCapsule()) {
                if (B.isBox()) {
                    ImpactResult result = isBoxAndCapsuleColliding(B, A);
                    
                    // Flip penetration direction as flipped inputs in previous call
                    result.penetrationDirection.flip();
                    return result;
                }
                if (B.isCapsule()) {
                    return isCapsuleAndCapsuleColliding(A, B);
                }
                if (B.isSphere()) {
                    return isCapsuleAndSphereColliding(A, B);
                }
            }
            if (A.isSphere()) {
                if (B.isBox()) {
                    ImpactResult result =  isBoxAndSphereColliding(B, A);

                    // Flip penetration direction as flipped inputs in previous call
                    result.penetrationDirection.flip();
                    return result;
                }
                if (B.isCapsule()) {
                    ImpactResult result =  isCapsuleAndSphereColliding(B, A);

                    // Flip penetration direction as flipped inputs in previous call
                    result.penetrationDirection.flip();
                    return result;
                }
                if (B.isSphere()) {
                    return isSphereAndSphereColliding(A, B);
                }
            }

            logger.logErrorMessage(
                "CollisionsComplex::isColliding",
                "Did not find a matching function for colliders A and B of types: "
                            + A.getTypeAsString() + ", " + B.getTypeAsString() 
            );
            return ImpactResult::noCollision();   
        }

        ImpactResult isBoxAndBoxColliding(const Collider& boxA, const Collider& boxB) {
            if (!boxA.isBox()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isBoxAndBoxColliding",
                    "Collider A was not a box but instead a " + boxA.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!boxB.isBox()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isBoxAndBoxColliding",
                    "Collider B was not a box but instead a " + boxA.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }

            /* SAT collision test
            Based on https://gamedev.stackexchange.com/questions/44500/how-many-and-which-axes-to-use-for-3d-obb-collision-with-sat/
            1. Calculate normals from rotation
            2. Calculate all corners from rotation
            3. Do SAT. Test 15 axes. If found a separating axis, can quit early as we know there's separation
              - Test each box normal (a0, a1, a2, b0, b1, b2)
              - cross(a0, b0)
              - cross(a0, b1)
              - cross(a0, b2)
              - cross(a1, b0)
              - cross(a1, b1)
              - cross(a1, b2)
              - cross(a2, b0)
              - cross(a2, b1)
              - cross(a2, b2)
            4. If intersecting on all axes, then finally return true as there's definitely collision
            */
            // FUTURE: Use the Real-Time Collision Detection algorithm, which is far more efficient but still a bit over my head

            // Create necessary data used for SAT tests
            std::vector<FPVector> aNormals = boxA.getBoxNormalsInWorldCoordinates();
            std::vector<FPVector> bNormals = boxB.getBoxNormalsInWorldCoordinates();
            std::vector<FPVector> aVertices = boxA.getBoxVerticesInWorldCoordinates();
            std::vector<FPVector> bVertices = boxB.getBoxVerticesInWorldCoordinates();

            // Estimate penetration depth by keeping track of shortest penetration axis
            fp smallestPenDepth = fp{-1};
            FPVector penDepthAxis;

            // Check for separating axis with a and b normals
            for (const FPVector& aNormal : aNormals) {
                if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, aNormal, smallestPenDepth,
                                                                  penDepthAxis)) {
                    return ImpactResult::noCollision();
                }
            }
            for (const FPVector& bNormal : bNormals) {
                if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, bNormal,smallestPenDepth,
                                                                  penDepthAxis)) {
                    return ImpactResult::noCollision();
                }
            }

            // Now check for axes based on cross products with each combination of normals
            // FUTURE: Probs could do the deal-with-square-at-end trick instead of full normalizations
            FPVector testAxis;
            testAxis = aNormals.at(0).cross(bNormals.at(0)).normalized(); // Normalize to fix pen depth calculations
            if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return ImpactResult::noCollision();
            }

            testAxis = aNormals.at(0).cross(bNormals.at(1)).normalized();
            if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return ImpactResult::noCollision();
            }

            testAxis = aNormals.at(0).cross(bNormals.at(2)).normalized();
            if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return ImpactResult::noCollision();
            }

            testAxis = aNormals.at(1).cross(bNormals.at(0)).normalized();
            if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return ImpactResult::noCollision();
            }

            testAxis = aNormals.at(1).cross(bNormals.at(1)).normalized();
            if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return ImpactResult::noCollision();
            }

            testAxis = aNormals.at(1).cross(bNormals.at(2)).normalized();
            if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return ImpactResult::noCollision();
            }

            testAxis = aNormals.at(2).cross(bNormals.at(0)).normalized();
            if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return ImpactResult::noCollision();
            }

            testAxis = aNormals.at(2).cross(bNormals.at(1)).normalized();
            if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return ImpactResult::noCollision();
            }

            testAxis = aNormals.at(2).cross(bNormals.at(2)).normalized();
            if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                              penDepthAxis)) {
                return ImpactResult::noCollision();
            }

            // Depth is supposed to be a sign-less magnitude. Thus flip penetration data if depth is currently negative
            if (smallestPenDepth < fp{0}) {
                smallestPenDepth *= fp{-1};
                penDepthAxis.flip();
            }

            // TODO: Flip penetration axis or not? Chosen standard is to be from boxA and face TOWARDS boxB (ie, towards the penetration itself)
            // Maybe view how others have approached this?

            /// Assure penetration axis is pointing in correct direction, ie it depicts direction that collider A is penetrating into collider B
            // Get direction from A to B center
            FPVector aToBDir = FPVector::direction(boxA.center, boxB.center);
            // Check if pen depth is NOT in "same" direction (ie, negative). Not caring about perpendicular (0) case, even if it's impossible
            if (penDepthAxis.dot(aToBDir) < fp{0}) {
                penDepthAxis.flip();
            }
            
            // We could find no separating axis, so definitely intersecting
            return ImpactResult(penDepthAxis, smallestPenDepth);
        }

        ImpactResult isCapsuleAndCapsuleColliding(const Collider& capA, const Collider& capB) {
            if (!capA.isCapsule()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isCapsuleAndCapsuleColliding",
                    "Collider A was not a capsule but instead a " + capA.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!capB.isCapsule()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isCapsuleAndCapsuleColliding",
                    "Collider B was not a capsule but instead a " + capB.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }

            // TODO: Document base logic from Real-Time Collisions 4.5.1 "TestCapsuleCapsule" method. Doc should be similar to other methods
            // TODO: Also rename vars

            // Get median line of capsules as points
            auto aLinePoints = capA.getCapsuleMedialLineExtremes();
            auto bLinePoints = capB.getCapsuleMedialLineExtremes();
            
            // Compute distance between the median line segments
            // "Compute (squared) distance between the inner structures of the capsules" (from book)
            fp intersectionTimeOnSegmentA, t;
            FPVector closestPtOnSegmentA, closestPtOnSegmentB;
            fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
                aLinePoints.start, aLinePoints.end,
                bLinePoints.start, bLinePoints.end,
                intersectionTimeOnSegmentA, t, closestPtOnSegmentA, closestPtOnSegmentB
            );

            // If (squared) distance smaller than (squared) sum of radii, they collide
            fp combinedRadius = capA.getCapsuleRadius() + capB.getCapsuleRadius();
            bool isColliding = distSquared < combinedRadius * combinedRadius;

            if (!isColliding) {
                return ImpactResult::noCollision();
            }

            FPVector penetrationDir;
            // Edge case: Closest points are the same point (ie, the capsule median lines overlap)
            // Note: Use isNear to cover math error range/minor inaccuracies. Eg, calculated closest point may be 0.000001 off
            if (closestPtOnSegmentA.isNear(closestPtOnSegmentB, fp{0.01f})) {
                // TODO: AH WAIT THIS IS INCORRECT UNDER SOME SCENARIOS ATM
                // Say colliders super fat but short plus they're parallel. Then one slides on top of another so their
                //      median lines overlap. The problem is that making them side by side is WAY TOO BIG compared to just
                //      moving them vertically.
                //      Need to decide whether should move perpendicular to median lines or along em...?
                
                // Penetration axis = Perpendicular to both capsule lines
                FPVector capsuleLineDirA = FPVector::direction(aLinePoints.start, aLinePoints.end);
                FPVector capsuleLineDirB = FPVector::direction(bLinePoints.start, bLinePoints.end);
                penetrationDir = capsuleLineDirA.cross(capsuleLineDirB); // TODO: Should cross be other way?
                
                // Edge case: Capsule lines are parallel, so use any perpendicular direction to the capsule lines
                if (penetrationDir.isNear(FPVector::zero(), fp{0.01f})) {
                    penetrationDir = VectorUtilities::getAnyPerpendicularVector(capsuleLineDirA);
                }
            }
            // Otherwise penetration direction = From one closest point to next
            else {
                penetrationDir = FPVector::direction(closestPtOnSegmentA, closestPtOnSegmentB);
            }
            
            // Penetration distance = distance between closest points on median lines minus sum of radii
            //                          (ie, how far to push in order to have the two touch side by side)
            fp penetrationDepth = FPMath::abs(FPMath::sqrt(distSquared) - combinedRadius);

            return ImpactResult(penetrationDir, penetrationDepth);
        }

        ImpactResult isSphereAndSphereColliding(const Collider& sphereA, const Collider& sphereB) {
            if (!sphereA.isSphere()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isSphereAndSphereColliding",
                    "Collider A was not a sphere but instead a " + sphereA.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!sphereB.isSphere()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isSphereAndSphereColliding",
                    "Collider B was not a sphere but instead a " + sphereB.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }

            // Solid explanation of core algorithm with pictures here:
            // https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection#bounding_spheres

            FPVector centerDifference = sphereB.getCenter() - sphereA.getCenter();
            fp centerDistance = centerDifference.getLength();
            fp intersectionDepth = (sphereA.getSphereRadius() + sphereB.getSphereRadius()) - centerDistance;

            if (intersectionDepth > fp{0}) {
                return ImpactResult(centerDifference.normalized(), intersectionDepth);
            }
            
            return ImpactResult::noCollision();
        }

        ImpactResult isBoxAndCapsuleColliding(const Collider& box, const Collider& capsule) {
            if (!box.isBox()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isBoxAndCapsuleColliding",
                    "Collider box was not a box but instead a " + box.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!capsule.isCapsule()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isBoxAndCapsuleColliding",
                    "Collider capsule was not a capsule but instead a " + capsule.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }

            // Base logic is from Real-Time Collisions 5.5.7

            // Compute line points for capsule
            auto worldSpaceCapsulePoints = capsule.getCapsuleMedialLineExtremes();
            // Convert line points from world space to local space so can continue with Capsule vs AABB approach
            FPVector boxSpaceCapsulePointA = box.toLocalSpaceFromWorld(worldSpaceCapsulePoints.start);
            FPVector boxSpaceCapsulePointB = box.toLocalSpaceFromWorld(worldSpaceCapsulePoints.end);
            Line boxSpaceCapsuleMedialSegment(boxSpaceCapsulePointA, boxSpaceCapsulePointB);

            // Compute the AABB resulting from expanding box faces by capsule radius
            Collider expandedCheckBox(box);
            expandedCheckBox.setCenter(FPVector::zero());  // Doing everything in box local space, so center should be at origin
            expandedCheckBox.setRotation(FPQuat::identity()); // Unnecessary but like being clear that this is an AABB (no rotation)
            expandedCheckBox.setBoxHalfSize(box.getBoxHalfSize() + FPVector(capsule.getCapsuleRadius()));
            
            fp timeOfIntersection;
            FPVector pointOfIntersection;
            bool didIntersect = getBoxCapsuleIntersection(
                box, expandedCheckBox, boxSpaceCapsuleMedialSegment, capsule.getCapsuleRadius(), capsule.getMedialHalfLineLength(),
                timeOfIntersection, pointOfIntersection
            );

            if (!didIntersect) {
                return ImpactResult::noCollision();
            }
            return calculateBoxCapsulePenetrationInfo(
                box, capsule, expandedCheckBox, boxSpaceCapsuleMedialSegment, timeOfIntersection, pointOfIntersection
            );
        }

        ImpactResult isBoxAndSphereColliding(const Collider& box, const Collider& sphere) {
            if (!box.isBox()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isBoxAndSphereColliding",
                    "Collider box was not a box but instead a " + box.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!sphere.isSphere()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isBoxAndSphereColliding",
                    "Collider sphere was not a sphere but instead a " + sphere.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }

            // Algorithm references:
            // AABB vs Sphere https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection#bounding_spheres
            // OOBB vs Sphere https://gamedev.stackexchange.com/a/163874/67844

            // Convert sphere's center to OBB's local space, with OOB's center as origin
            // This allows us to use simplified Sphere vs AABB logic here on out
            FPVector localSphereCenter = box.toLocalSpaceFromWorld(sphere.getCenter());

            // Find the closest point on the box to the sphere's center
            // Essentially just uses sphere center coordinate on each axis IF within box, otherwise clamps to nearest box extent
            // Suggest visualizing with number lines
            const FPVector& extents = box.getBoxHalfSize(); // Represents min and max points, since using box center as origin
            FPVector closestBoxPointToSphere;
            closestBoxPointToSphere.x = FPMath::max(-extents.x, FPMath::min(localSphereCenter.x, extents.x));
            closestBoxPointToSphere.y = FPMath::max(-extents.y, FPMath::min(localSphereCenter.y, extents.y));
            closestBoxPointToSphere.z = FPMath::max(-extents.z, FPMath::min(localSphereCenter.z, extents.z));

            // Finally do ordinary point distance vs sphere radius checking
            FPVector closestPointOffSetToSphere = localSphereCenter - closestBoxPointToSphere;
            fp sphereCenterToBoxDistance = closestPointOffSetToSphere.getLength();

            // Edge case: Sphere center within box. Confirmed collision but need to calculate penetration info in a different way
            if (sphereCenterToBoxDistance == fp{0}) {
                // FUTURE: Something feels off with below method vs ordinary method
                //          Why does ordinary case resolve successfully in an arbitrary (sphere) direction, while here
                //              we have to adhere to box normals?
                //          Feels like there should be a smoother + more accurate (and likely more efficient) answer
                // Albeit maybe I'm overthinking this, as it's pretty smooth when moving a small sphere through a big box
                
                // Calculate direction and distance necessary to push sphere's center out of box and to the box's face
                FPVector dirToPushSphereOutToBoxFace;
                fp distanceToPushSphereCenterOutOfBox;
                calculateSmallestPushToOutsideBox(box, localSphereCenter, dirToPushSphereOutToBoxFace, distanceToPushSphereCenterOutOfBox);

                // To clear collision, the sphere needs to be pushed further until its surface only just touches the box
                fp penetrationDistance = distanceToPushSphereCenterOutOfBox + sphere.getSphereRadius();
                
                // Finally return result
                // Note that NOT flipping face normal as this follows penetration axis standard (ie, direction that first collider is penetrating into B)
                return ImpactResult(dirToPushSphereOutToBoxFace, penetrationDistance);
            }

            // Calculate how much sphere is intersecting into the box, if at all
            fp intersectionDepth = sphere.getSphereRadius() - sphereCenterToBoxDistance;
            if (intersectionDepth > fp{0}) {
                return ImpactResult(closestPointOffSetToSphere.normalized(), intersectionDepth);
            }
            
            return ImpactResult::noCollision();
        }

        ImpactResult isCapsuleAndSphereColliding(const Collider& capsule, const Collider& sphere) {
            if (!capsule.isCapsule()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isCapsuleAndSphereColliding",
                    "Collider capsule was not a capsule but instead a " + capsule.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!sphere.isSphere()) {
                logger.logErrorMessage(
                    "CollisionsComplex::isCapsuleAndSphereColliding",
                    "Collider sphere was not a sphere but instead a " + sphere.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }

            /// Note: Base logic taken from Real-Time Collisions 4.5.1 "TestSphereCapsule" method

            // Get median line of capsule as points
            auto capsulePoints = capsule.getCapsuleMedialLineExtremes();

            // Get closest point on line segment between sphere and capsule
            // Need point instead of distance (getSquaredDistBetweenPtAndSegment) for penetration info later on
            FPVector closestPtOnCapsuleLine;
            fp throwaway;
            CollisionHelpers::getClosestPtBetweenPtAndSegment(capsulePoints, sphere.center,
                                                            throwaway, closestPtOnCapsuleLine);
            
            // Compute distance between closest point on capsule line vs sphere center
            // Using square to avoid potentially unnecessary square root
            fp distSquared = FPVector::distanceSq(closestPtOnCapsuleLine, sphere.center);

            // If (squared) distance smaller than (squared) sum of radii, they collide
            fp combinedRadius = sphere.getSphereRadius() + capsule.getCapsuleRadius();
            bool isColliding = distSquared < combinedRadius * combinedRadius;
            
            if (!isColliding) {
                return ImpactResult::noCollision();
            }

            FPVector penetrationDir;
            // EDGE CASE: If sphere center == closest capsule point (ie, on capsule median line)...
            // Note: Use isNear to cover math error range/minor inaccuracies. Eg, calculated closest point may be 0.000001 off
            if (sphere.getCenter().isNear(closestPtOnCapsuleLine, fp{0.01f})) {
                // Choose any penetration direction perpendicular to capsule line as that's guaranteed to push colliders away
                FPVector capsuleLineDir = FPVector::direction(capsulePoints.start, capsulePoints.end);
                penetrationDir = VectorUtilities::getAnyPerpendicularVector(capsuleLineDir);
            }
            // Otherwise penetration direction = From sphere center towards closest point on capsule, as this is most
            //                          direct method to push capsule/sphere out of collision
            else {
                penetrationDir = FPVector::direction(closestPtOnCapsuleLine, sphere.getCenter());
            }
            
            // Penetration distance = distance between closest point on line vs sphere center minus sum of radii
            //                          (ie, how far to push in order to have the two touch side by side)
            fp penetrationMagnitude = FPMath::abs(FPMath::sqrt(distSquared) - combinedRadius);
            
            return ImpactResult(penetrationDir, penetrationMagnitude);
        }
        
#pragma endregion

        // GJK isColliding implementation
        bool isCollidingViaGJK(const Collider& colliderA, const Collider& colliderB) {
            // TODO: May have a bug if origin is contained in one of the edges or faces. According to comments, due to one of the >= ..?

            if (colliderA.isNotInitialized()) {
                logger.logErrorMessage(
                    "ComplexCollisions::isCollidingViaGJK",
                    "Collider A was not initialized type"
                );
                return false;
            }
            if (colliderB.isNotInitialized()) {
                logger.logErrorMessage(
                    "ComplexCollisions::isCollidingViaGJK",
                    "Collider B was not initialized type"
                );
                return false;
            }
            
            // If sphere-sphere, calculate via classic method
            // Necessary as current EPA implementation will result in an inf loop. Plus this is much faster
            if (colliderA.isSphere() && colliderB.isSphere()) {
                return isSphereAndSphereColliding(colliderA, colliderB).isColliding;
            }
            
            Simplex throwaway;
            return isCollidingViaGJK(colliderA, colliderB, throwaway);
        }

        /// <summary>
        /// Check if two colliders are colliding, and if so, return additional details on the collision. Useful for collision resolution.
        /// Uses GJK + EPA algorithms. TODO: Useful resources on those algorithms
        /// TODO: Rename to isColliding after cleaning up other older functions
        /// </summary>
        /// <param name="colliderA">First collider to check for collision with</param>
        /// <param name="colliderB">Second collider to check for collision with</param>
        /// <returns>Returns if collision occurs, and if so, also returns additional information on collision</returns>
        ImpactResult computeComplexCollision(const Collider& colliderA, const Collider& colliderB) {
            // TODO: Test if capsule-capsule works as well as sphere vs edge,
            //     as this implementation doesn't like curves (link to YT comment). May need to set hard limit on iterations as quick fix

            if (colliderA.isNotInitialized()) {
                logger.logErrorMessage(
                    "ComplexCollisions::computeComplexCollision",
                    "Collider A was not initialized type"
                );
                return ImpactResult::noCollision();
            }
            if (colliderB.isNotInitialized()) {
                logger.logErrorMessage(
                    "ComplexCollisions::computeComplexCollision",
                    "Collider B was not initialized type"
                );
                return ImpactResult::noCollision();
            }
            
            // If sphere-sphere, calculate via classic method
            // Necessary as current EPA implementation will result in an inf loop. Plus this is much faster
            if (colliderA.isSphere() && colliderB.isSphere()) {
                return isSphereAndSphereColliding(colliderA, colliderB);
            }
            
            Simplex gjkResultingSimplex;
            bool isColliding = isCollidingViaGJK(colliderA, colliderB, gjkResultingSimplex);
            
            if (!isColliding) {
                return ImpactResult::noCollision();
            }
            
            return computeCollisionInfoViaEPA(colliderA, colliderB, gjkResultingSimplex);
        }
        
    private:
#pragma region Complex Collision Check Helpers

        // Collision resolution purpose: Have a point in space within box and want to find which direction to push it,
        //                                  such that it's the smallest direction to push the point out of the box
        // NOTE: outPushToBoxFaceDistance is >= 0 (ie, is a magnitude and should not be negative)
        void calculateSmallestPushToOutsideBox(const Collider& box,
                                                const FPVector& localSpacePoint,
                                                FPVector& outPushToOutsideBoxDirInWorldSpace,
                                                fp& outPushToBoxFaceDistance,
                                                bool useWrongDirectionFiltering = false,
                                                const FPVector& directionForFiltering = FPVector::zero()) {
            // Declare variables
            fp smallestDistToPushSoFar = FPMath::maxLimit();
            FPVector bestPushDirSoFar = FPVector::zero();   // At time of writing this is guaranteed to be set again, but best to be safe
            FPVector boxHalfSize = box.getBoxHalfSize();    // Cache so don't have to recreate every time (given current implementation)

            // Check each axis (pair of faces) for closest face distance to push point to
            checkIfFaceAlongAxisIsClosestToPoint(
                boxHalfSize, FPVector::forward(), localSpacePoint, smallestDistToPushSoFar, bestPushDirSoFar,
                useWrongDirectionFiltering, directionForFiltering
            );
            checkIfFaceAlongAxisIsClosestToPoint(
                boxHalfSize, FPVector::right(), localSpacePoint, smallestDistToPushSoFar, bestPushDirSoFar,
                useWrongDirectionFiltering, directionForFiltering
            );
            checkIfFaceAlongAxisIsClosestToPoint(
                boxHalfSize, FPVector::up(), localSpacePoint, smallestDistToPushSoFar, bestPushDirSoFar,
                useWrongDirectionFiltering, directionForFiltering
            );

            // NOTE: No need to check edges, as edges will never be shorter distance than pushing out to a face
            //       Easy check: Pythagoras. An edge is like the hypotenuse with faces are other parts of triangle/pyramid,
            //                      and hypotenuse is always greater than the other parts
            
            // Set results
            outPushToBoxFaceDistance = smallestDistToPushSoFar; // Magnitude so don't need to convert to world space
            outPushToOutsideBoxDirInWorldSpace = box.toWorldSpaceForOriginCenteredValue(bestPushDirSoFar);
        }

        // curAxisDir = Up, Right, or Forward vectors as working in box local space
        void checkIfFaceAlongAxisIsClosestToPoint(const FPVector& boxHalfSize,
                                                    const FPVector& curAxisDir,
                                                    const FPVector& pointToPush,
                                                    fp& smallestDistSoFar,
                                                    FPVector& bestPushDirSoFar,
                                                    bool useWrongDirectionFiltering,
                                                    const FPVector& directionForFiltering) {
            fp pointExtentInAxis = curAxisDir.dot(pointToPush);      // Eg, Up.dot(Px, Py, Pz) will result in Pz
            fp distanceToFace;

            // First check if face along positive axis direction is the closest face to our point thus far
            if (!useWrongDirectionFiltering || directionForFiltering.dot(curAxisDir) >= fp{0}) {
                fp extentInPosFaceDir = curAxisDir.dot(boxHalfSize);  // Eg, Up.dot(HalfSize) will result in zMax
                distanceToFace = FPMath::abs(extentInPosFaceDir - pointExtentInAxis);
                
                if (distanceToFace < smallestDistSoFar) {
                    // Remember this face as the closest face thus far
                    smallestDistSoFar = distanceToFace;
                    bestPushDirSoFar = curAxisDir;
                }
            }
            
            // Next check if face along negative axis direction is the closest face to our point thus far
            // Note that can't optimize this out as one direction may be closer than before but then other direction is even closer
            if (!useWrongDirectionFiltering || directionForFiltering.dot(-curAxisDir) >= fp{0}) {
                FPVector negAxisDir = curAxisDir.flipped();
                fp extentInNegFaceDir = negAxisDir.dot(boxHalfSize);  // Eg, Down * HalfSize will result in -zMax
                distanceToFace = FPMath::abs(extentInNegFaceDir - pointExtentInAxis);
                
                if (distanceToFace < smallestDistSoFar) {
                    // Remember this face as the closest face thus far
                    smallestDistSoFar = distanceToFace;
                    bestPushDirSoFar = negAxisDir;
                }
            }
        }

        bool getBoxCapsuleIntersection(const Collider& box,
                                        const Collider& expandedCheckBox,
                                        const Line& boxSpaceCapsuleMedialSegment,
                                        const fp& capsuleRadius,
                                        const fp& capsuleMedialHalfLineLength,
                                        fp& timeOfIntersection,
                                        FPVector& pointOfIntersection) {
            // Shortcut variables
            const FPVector& boxSpaceCapsulePointA = boxSpaceCapsuleMedialSegment.start;
            const FPVector& boxSpaceCapsulePointB = boxSpaceCapsuleMedialSegment.end;

            // Intersect ray against expanded box. Exit with no intersection if ray misses box, else get intersection point and time as result
            // NOTE: Pretty certain didn't need to convert to box space for this test (since raycast does that)
            //          Minor optimization: Do this raycast test *before* converting to box local space
            //          HOWEVER, may not work due to following math (and if not possible, then update these comments so don't go down this path again)
            Ray intersectionTestRay = Ray::fromPoints(boxSpaceCapsulePointA, boxSpaceCapsulePointB);
            bool didRaycastIntersectCheckBox = simpleCollisions.raycastWithBox(
                intersectionTestRay, expandedCheckBox,timeOfIntersection, pointOfIntersection
            );
            // If raycast did not hit at all then definitely no collision
            if (!didRaycastIntersectCheckBox) {
                return false;
            }
            // Verify that raycast intersection point is within range of the capsule median line
            // (ie, turn this into a linetest)
            timeOfIntersection = timeOfIntersection / (capsuleMedialHalfLineLength * 2); // Raycast "time" is actually distance based
            if (timeOfIntersection >= fp{1}) {
                // Edge case: Median line is within the expanded box but doesn't intersect with the surface of the box.
                //              Raycast usage will not catch this case so check for it explicitly 
                if (expandedCheckBox.isLocalSpacePtWithinBoxExcludingOnSurface(boxSpaceCapsulePointA)) {
                    // Default to final capsule median line point for further calculations
                    // Side note: Not sure whether to pick initial or final point, but given there's a min operation
                    //              operation with time later on, it's safest to use the maximum time. No idea if matters
                    timeOfIntersection = fp{1}; // Latter linetest considers 1 = 100% of line length. Yes this is inconsistent with raycast
                    pointOfIntersection = boxSpaceCapsulePointB;
                }
                    // Otherwise the box is certainly too far for an intersection
                else {
                    return false;
                }
            }

            // Compute which min and max faces of box the intersection point lies outside of.
            // Note, the two vars cannot have the same bits set and they must have at least one bit set among them
            uint32_t lessThanMinExtentChecks = 0, greaterThanMaxExtentChecks = 0;
            FPVector minBoxExtents = -box.getBoxHalfSize();
            FPVector maxBoxExtents = box.getBoxHalfSize();
            if (pointOfIntersection.x < minBoxExtents.x) lessThanMinExtentChecks |= 1;
            if (pointOfIntersection.x > maxBoxExtents.x) greaterThanMaxExtentChecks |= 1;
            if (pointOfIntersection.y < minBoxExtents.y) lessThanMinExtentChecks |= 2;
            if (pointOfIntersection.y > maxBoxExtents.y) greaterThanMaxExtentChecks |= 2;
            if (pointOfIntersection.z < minBoxExtents.z) lessThanMinExtentChecks |= 4;
            if (pointOfIntersection.z > maxBoxExtents.z) greaterThanMaxExtentChecks |= 4;

            // "Or" all set bits together into a bit mask (note: effectively here u + v == u | v as same bit can't be set in both variables)
            uint32_t mask = lessThanMinExtentChecks + greaterThanMaxExtentChecks;

            // If all 3 bits set (m == 7) then intersection point (if any) is in a vertex region
            if (mask == 7) {
                bool didIntersect;
                
                // Must now intersect capsule line segment against the capsules of the three
                // edges meeting at the vertex and return the best time, if one or more hit
                fp tMin = FPMath::maxLimit();
                FPVector tMinIntersectionPoint;

                // Note that endpoint of test line will be changed for each test
                Line testCapsuleMedianLine(
                    simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks), 
                    FPVector::zero()
                );

                testCapsuleMedianLine.end = simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks ^ 1);
                didIntersect = simpleCollisions.linetestWithCapsule(
                    boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                    capsuleRadius, timeOfIntersection, pointOfIntersection
                );
                if (didIntersect && timeOfIntersection < tMin) {
                    tMin = timeOfIntersection;
                    tMinIntersectionPoint = pointOfIntersection;
                }

                testCapsuleMedianLine.end = simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks ^ 2);
                didIntersect = simpleCollisions.linetestWithCapsule(
                    boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                    capsuleRadius, timeOfIntersection, pointOfIntersection
                );
                if (didIntersect && timeOfIntersection < tMin) {
                    tMin = timeOfIntersection;
                    tMinIntersectionPoint = pointOfIntersection;
                }

                testCapsuleMedianLine.end = simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks ^ 4);
                didIntersect = simpleCollisions.linetestWithCapsule(
                    boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                    capsuleRadius, timeOfIntersection, pointOfIntersection
                );
                if (didIntersect && timeOfIntersection < tMin) {
                    tMin = timeOfIntersection;
                    tMinIntersectionPoint = pointOfIntersection;
                }

                // If didn't find a single intersection, then confirmed that there was no intersection
                if (tMin == FPMath::maxLimit()) {
                    return false;
                }

                timeOfIntersection = tMin;
                pointOfIntersection = tMinIntersectionPoint;
                return true;
            }
            
            // If only one bit set in m, then intersection point is in a face region
            if ((mask & (mask - 1)) == 0) {
                // Do nothing. Time t from intersection with expanded box is correct intersection time
                // TODO: Comment is incorrect, as doesn't work for case where median line within extended capsule
                return true;
            }
            
            // p is in an edge region. Intersect against the capsule at the edge
            Line testCapsuleMedianLine(
                simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, lessThanMinExtentChecks ^ 7),
                simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks)
            );
            bool didIntersect = simpleCollisions.linetestWithCapsule(
                boxSpaceCapsuleMedialSegment,
                testCapsuleMedianLine,
                capsuleRadius,
                timeOfIntersection,
                pointOfIntersection
            );
            
            return didIntersect;
        }

        ImpactResult calculateBoxCapsulePenetrationInfo(const Collider& box,
                                                        const Collider& capsule,
                                                        const Collider& expandedCheckBox,
                                                        const Line& boxSpaceCapsuleMedialLine,
                                                        const fp& timeOfInitialIntersection,
                                                        const FPVector& pointOfInitialIntersection) {
            if (!box.isBox()) {
                logger.logErrorMessage(
                    "ComplexCollisions::calculateBoxCapsulePenetrationInfo",
                    "Provided collider was not a box but instead a " + box.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!expandedCheckBox.isBox()) {
                logger.logErrorMessage(
                    "ComplexCollisions::calculateBoxCapsulePenetrationInfo",
                    "Provided checkAgainstBox collider was not a box but instead a " + box.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!capsule.isCapsule()) {
                logger.logErrorMessage(
                    "ComplexCollisions::calculateBoxCapsulePenetrationInfo",
                    "Collider capsule was not a capsule but instead a " + capsule.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }

            // Check if either capsule medial line endpoints are within the expanded box.
            //      Specifically checking expanded box as it includes capsule radius (eg, catch case where capsule median
            //      line is right outside original box but intersecting due to the width of the capsule itself)
            // TODO: Check if there are any inaccuracies stemming from expanded box corners vs normal box
            bool isCapsuleLineStartInExpandedBox = expandedCheckBox.isLocalSpacePtWithinBoxExcludingOnSurface(boxSpaceCapsuleMedialLine.start);
            bool isCapsuleLineEndInExpandedBox = expandedCheckBox.isLocalSpacePtWithinBoxExcludingOnSurface(boxSpaceCapsuleMedialLine.end);

            /// Choose approach depending on number of endpoints within box
            //  Note that standard is for pen direction to point from one object INTO other object,
            //      and specifically from box's perspective (as box-capsule method has box as first 
            FPVector boxPenetrationDirInWorldSpace = FPVector::zero();
            fp penetrationMagnitude = fp{0};
            FPVector capsuleLineDir = boxSpaceCapsuleMedialLine.getDirection();
            if (isCapsuleLineStartInExpandedBox && isCapsuleLineEndInExpandedBox) {
                // Approach: We have 2 options for getting capsule out of box with smallest possible movement:
                // 1. Parallel to line direction: Move first medial line endpoints towards other endpoint and out of box
                // 2. Opposite line direction: Move second endpoint towards first endpoint and out of box
                // Calculate both and choose the one with smallest movement

                // Thus, the approach here is to solve for all three approaches then choose one with smallest distance
                // Approach 1: Try moving start endpoint to closest face in general line direction
                FPVector fromStartPenetrationDirInWorldSpace;
                fp fromStartPenetrationMagnitude;
                calculateSmallestPushToOutsideBox(
                    expandedCheckBox, boxSpaceCapsuleMedialLine.start, fromStartPenetrationDirInWorldSpace, fromStartPenetrationMagnitude,
                    true, capsuleLineDir
                );

                // Approach 2: Try moving end endpoint to closest face in opposite line direction
                FPVector fromEndPenetrationDirInWorldSpace;
                fp fromEndPenetrationMagnitude;
                calculateSmallestPushToOutsideBox(
                    expandedCheckBox, boxSpaceCapsuleMedialLine.end, fromEndPenetrationDirInWorldSpace, fromEndPenetrationMagnitude,
                    true, capsuleLineDir.flipped()
                );

                // Finally choose the push method with the smallest distance
                if (fromEndPenetrationMagnitude < fromStartPenetrationMagnitude) {
                    penetrationMagnitude = fromEndPenetrationMagnitude;
                    boxPenetrationDirInWorldSpace = fromEndPenetrationDirInWorldSpace;
                }
                else {
                    penetrationMagnitude = fromStartPenetrationMagnitude;
                    boxPenetrationDirInWorldSpace = fromStartPenetrationDirInWorldSpace;
                }
            }
            else if (isCapsuleLineStartInExpandedBox) {
                // Simple approach: Move endpoint to closest face while ignoring faces opposite of line direction
                calculateSmallestPushToOutsideBox(
                    expandedCheckBox, boxSpaceCapsuleMedialLine.start, boxPenetrationDirInWorldSpace, penetrationMagnitude,
                    true, capsuleLineDir
                );
            }
            else if (isCapsuleLineEndInExpandedBox) {
                // Simple approach: Move endpoint to closest face while ignoring faces opposite of line direction
                calculateSmallestPushToOutsideBox(
                    expandedCheckBox, boxSpaceCapsuleMedialLine.end, boxPenetrationDirInWorldSpace, penetrationMagnitude,
                    true, capsuleLineDir.flipped()
                );
            }
            // Otherwise no medial line endpoint within box... 
            else {
                // Given we need to move the ENTIRE intersection line of the box but intersection is in middle of capsule,
                //      we'll need to focus on the best direction to move the intersection itself.
                // Note that need to move the middle point of the intersection line (and perpendicularly to line),
                //      as it guarantees all intersecting points will exit the box.
                // Approach:
                // 1. Get middle point of intersection
                // 2. Find closest face to move middle point out of box
                // 3. Get perpendicular-to-line direction that closest matches the closest face
                // 4. Calculate distance via raycast method

                /// 1. Get middle point of intersection
                // Get intersection point when line is reversed
                FPVector pointOfLastIntersection;
                fp throwaway;
                Line reversedMedialLine(boxSpaceCapsuleMedialLine.end, boxSpaceCapsuleMedialLine.start);
                getBoxCapsuleIntersection(
                    box, expandedCheckBox, reversedMedialLine, capsule.getCapsuleRadius(), capsule.getMedialHalfLineLength(),
                    throwaway, pointOfLastIntersection
                );
                // Finally calculate middle of intersection
                FPVector middleIntersectionPoint = (pointOfInitialIntersection + pointOfLastIntersection) / fp{2};

                /// Steps 2-4 will be handled by following method:
                getBestPushInfoOutOfBoxForMiddlePointOfBoxSpaceLine(
                    expandedCheckBox, middleIntersectionPoint, capsuleLineDir, boxPenetrationDirInWorldSpace, penetrationMagnitude
                );
            }
            
            return ImpactResult(boxPenetrationDirInWorldSpace, penetrationMagnitude);
        }

        /// <summary>Calculate which direction to push an intersecting line out of a box with minimum distance</summary>
        /// <param name="box">Box to push line out of</param>
        /// <param name="middleIntersectionPoint">
        /// Middle of intersection of line. Note that this isn't necessarily the middle point of an ENTIRE line segment but
        /// the merely middle of the segment of the line which intersects the box. 
        /// </param>
        /// <param name="lineDir">Normalized direction of the line</param>
        /// <param name="bestDirToPushLineOutOfBox">Output variable, direction to push middle point out of box</param>
        /// <param name="penetrationMagnitude">Output variable, how much to push middleIntersectionPoint along output direction</param>
        void getBestPushInfoOutOfBoxForMiddlePointOfBoxSpaceLine(const Collider& box,
                                                                const FPVector& middleIntersectionPoint,
                                                                const FPVector& lineDir,
                                                                FPVector& bestDirToPushLineOutOfBox,
                                                                fp& penetrationMagnitude) {
            // Find closest face to move middle point out of box
            FPVector smallestPushToFaceDir;
            fp throwaway;
            calculateSmallestPushToOutsideBox(
                box, middleIntersectionPoint, smallestPushToFaceDir, throwaway
            );

            // Get perpendicular-to-line direction that closest matches the closest-face direction
            // Based on following: https://math.stackexchange.com/a/410549/815287
            FPVector bestMovementDir = smallestPushToFaceDir.cross(lineDir).cross(lineDir);

            /// Calculate distance via raycast method
            // As this is all in local space, we will directly use the AABB method for raycast testing
            Ray testRay(middleIntersectionPoint, bestMovementDir);
            FPVector raycastIntersectionPoint;
            fp raycastIntersectionTime;
            simpleCollisions.raycastForAABB(testRay, box, raycastIntersectionTime, raycastIntersectionPoint);

            // Finally set the well-earned results
            bestDirToPushLineOutOfBox = box.toWorldSpaceForOriginCenteredValue(bestMovementDir);
            penetrationMagnitude = raycastIntersectionTime; // Raycast "time" is actually equivalent to distance
        }
        
#pragma endregion 
#pragma region GJK Internal
        
        /// <summary>
        /// GJK isColliding implementation
        /// Based on the following: https://youtu.be/MDusDn8oTSE?t=284
        /// In fact, *all* of this logic - including terminology - is heavily based on this vid instead: https://youtu.be/Qupqu1xe7Io
        /// However, for a high-level introduction to how the GJK algorithm works, I highly HIGHLY recommend
        ///     watching the following first: https://youtu.be/SDS5gLSiLg0?t=2648
        /// </summary>
        /// <param name="colliderA">First collider to check for collision with</param>
        /// <param name="colliderB">Second collider to check for collision with</param>
        /// <param name="resultSimplex">Resulting simplex from GJK algorithm. Only set if collision found</param>
        /// <returns>Returns true if collision found, false otherwise</returns>
        bool isCollidingViaGJK(const Collider& colliderA, const Collider& colliderB, Simplex& resultSimplex) {
            // Get initial support point in any direction
            // Could try to be smart here with initial direction choice, but GJK is good at converging at an answer so not a large concern
            // TODO: Run all unit tests with an odd starting direction which broke tests before: FPVector(fp{1}, fp{1}, fp{1}).normalized()
            FPVector supportPoint = getSupportPoint(colliderA, colliderB, FPVector::forward());

            // Simplex is an array of points, max count is 4
            Simplex points;
            points.pushFront(supportPoint);

            // New search direction is towards the origin (as we're trying to build a simplex that contains the origin to prove collision)
            // FUTURE OPTIMIZATION: Directions don't need to be normalized EXCEPT in getSupportPoint() as we otherwise ONLY do dot + cross products and check sign
            //                      Thus, if we only normalize in getSupportPoint (and better name everything), then we don't need all these common expensive normalizations!
            FPVector direction = -supportPoint.normalized();

            // Keep searching for origin until we either contain it within the simplex OR verify that we cannot contain the origin
            uint32_t iterationCount = 0;
            while (true) {
                iterationCount++;
                
                // Calculate next best support point for simplex
                // (I think simplex/support points are supposed to represent best estimate of Minkowski difference?)
                supportPoint = getSupportPoint(colliderA, colliderB, direction);

                if (supportPoint.dot(direction) <= fp{0}) { // If next best guess for simplex support point didn't cross the origin...
                    // Then no collision, as we've already searched as far as we could for origin
                    // Note that this assumes that search direction WAS (generally) towards origin, which it should be for the sake of this algorithm
                    return false;
                }

                // Add new support point to our simplex thus far
                // Push it to the front as other calculations will assume "front" point is latest best-guess point
                points.pushFront(supportPoint);

                // Finally do our super duper secret "doSimplex" code which updates simplex AND search direction, which also checks if collision found
                if (checkNextSimplex(points, direction)) {
                    resultSimplex = points;
                    return true;
                }
            }
        }
        
        // Support function for GJK + EPA. Assists in determining supporting points for simplex + polytope creation
        // Used in many GJK examples, but directly based on https://youtu.be/MDusDn8oTSE?t=187
        FPVector getSupportPoint(const Collider& colliderA, const Collider& colliderB, const FPVector& direction) {
            return colliderHelpers.getFurthestPoint(colliderA, direction)
                    - colliderHelpers.getFurthestPoint(colliderB, -direction);
        }

        // Simple helper function to determine if two directions are along the same direction
        bool isSameDirection(const FPVector& direction, const FPVector& aToOriginDir) {
            return direction.dot(aToOriginDir) > fp{0};
        }

        /// <summary>Do some work depending on current simplex shape</summary>
        /// <returns>True if collision is confirmed (ie, origin contained within simplex), false if still not sure</returns>
        bool checkNextSimplex(Simplex& points, FPVector& direction) {
            // Compute simplex and relevant checks based on current simplex shape
            // Note that generally just does two things:
            //  1. If tetrahedron and origin is contained in it, return true
            //  2. Otherwise, looks to see which part of shape is closest to origin then modifies simplex to try to contain origin
            //              AND sets search direction from that new simplex to the origin
            // Other notes:
            //      - New point is commonly referred to as A, with B the second to last point added to the simplex
            //      - ...and yes, much of GJK algorithm is based on knowing which points were in the simplex before
            switch (points.size()) {
                case 2: return onLineSimplexCheck(points, direction);
                case 3: return onTriangleSimplexCheck(points, direction);
                case 4: return onTetrahedronSimplexCheck(points, direction); // Should never need to go past 4 points to contain origin
                default:
                    // Never should get here
                    logger.logWarnMessage(
                        "ComplexCollisions::checkNextSimplex",
                        "Reached supposedly unreachable point. Number of points: " + std::to_string(points.size())
                    );
                    return false;
            }
        }

        /// <summary>Line case logic for pruning or verifying simplex then calculating next best search direction</summary>
        /// <param name="points">
        /// Input/output variable. Outputs either same input simplex or pruned simplex for best guess at containing origin
        /// </param>
        /// <param name="outSearchDirection">
        /// Output variable. Represents next best guess for calculating simplex point that would help contain origin
        /// </param>
        /// <returns>Always returns false. (True would signify collision verified, which is impossible without 4 points in 3D)</returns>
        bool onLineSimplexCheck(Simplex& points, FPVector& outSearchDirection) {
            // https://youtu.be/MDusDn8oTSE?t=330
            FPVector a = points[0];
            FPVector b = points[1];

            // Make all rays in relation to A as makes algorithm easier (see next block of comments)
            // FUTURE OPTIMIZATION: Directions don't need to be normalized EXCEPT in getSupportPoint() as we otherwise ONLY do dot + cross products and check sign
            //                      Thus, if we only normalize in getSupportPoint (and better name everything), then we don't need all these common expensive normalizations!
            FPVector aToB = (b - a).normalized();
            FPVector aToOrigin = (-a).normalized();

            // Determine which part is closest to origin, which could be three parts:
            // Case 1: Point B is closest to origin (on line AB, so origin is beyond B)
            // Case 2: Point A is closest to origin (on line AB, so origin is before A)
            // Case 3: Origin is somewhere "along" AB, so origin is BETWEEN AB
            // 
            // In popular implementations, we assume that Case 1 (B closest point) is impossible as we JUST added A- and we always add points towards origin
            //      However, in reality that's not true (perhaps due to add direction not always towards origin). We still need to check that case

            // Case 1: Is origin beyond B?
            FPVector bToOrigin = (-b).normalized();
            if (isSameDirection(aToB, bToOrigin)) {
                // TEMP: Observation if hitting these cases that simplified GJK algo said we could safely skip
                logger.logWarnMessage(
                    "ComplexCollisions::onLineSimplexCheck",
                    "Reached supposedly unnecessary line case"
                );
                
                // Throw out A as it didn't get us closer to the origin
                points = { b };
                outSearchDirection = bToOrigin;
            }
            // Case 3: Is origin in same direction as AB? (and - given Case 1 can't occur - then origin between A and B)
            else if (isSameDirection(aToB, aToOrigin)) {
                // If so, prepare to add next simplex point via computing a new search direction perpendicular to AB and towards origin
                outSearchDirection = aToB.cross(aToOrigin).cross(aToB).normalized();
            }
            // Otherwise origin is not in general direction of A to B (thus Case 2), so throw out B and prepare to recalculate line simplex
            else {
                points = { a };
                outSearchDirection = aToOrigin;
            }

            return false; // Can't confirm if origin is contained within simplex with just a line, so return false
        }

        /// <summary>Triangle case logic for pruning or verifying simplex then calculating next best search direction</summary>
        /// <param name="points">
        /// Input/output variable. Outputs either same input simplex or pruned simplex for best guess at containing origin
        /// </param>
        /// <param name="outSearchDirection">
        /// Output variable. Represents next best guess for calculating simplex point that would help contain origin
        /// </param>
        /// <returns>Always returns false. (True would signify collision verified, which is impossible without 4 points in 3D)</returns>
        bool onTriangleSimplexCheck(Simplex& points, FPVector& outSearchDirection) {
            // https://youtu.be/MDusDn8oTSE?t=365
            FPVector a = points[0];
            FPVector b = points[1];
            FPVector c = points[2];

            FPVector aToB = (b - a).normalized();
            FPVector aToC = (c - a).normalized();
            FPVector aToOrigin = (-a).normalized();

            FPVector abc = aToB.cross(aToC).normalized(); // Represents outwards direction perpendicular to AC face

            // Similar to line test, where essentially doing a bunch of plane tests to find which side of our triangle the origin lies
            // Recommend drawing this out, such as similar diagram to https://youtu.be/MDusDn8oTSE?t=370
            //
            // 8 possible areas that origin could be closest to:
            // Case 1: Closest to A
            // Case 2: Closest to B
            // Case 3: Closest to C
            // Case 4: Perpendicular to AB and away from triangle ABC
            // Case 5: Perpendicular to AC and away from triangle ABC
            // Case 6: Perpendicular to BC and away from triangle ABC
            // Case 7: "Above" (perpendicular to) triangle ABC
            // Case 8: "Below" (perpendicular to) triangle ABC
            //
            // In popular implementations, we assume that Cases 2 + 3 + 6 are impossible as origin MUSt be away from that side of triangle.
            //      However, in reality that's not true (perhaps due to add direction not always towards origin). We still need to check that case
            // Thus, right now we'll just be checking if origin is first outside of the triangle edge-by-edge
            // Note that goal is to create and verify a triangle that contains the origin. Only thereafter will we move to tetrahedron

            // Edge check: Is origin towards ABC x AC (outside of edge AC away from the triangle)?
            if (isSameDirection(abc.cross(aToC), aToOrigin)) {
                // Possibly cases 1 (A), 3 (C), or 5 (away from AC between A and C). Throwing out B from points for sure since origin not around there
                // So - depending on which side of our triangle the origin is exactly - are we also throwing out A, C, or keeping both?
                // Ya know what's this logic exactly?
                // The LineSimplex code! Can just reuse that here thankfully!
                return onLineSimplexCheck(points = { a, c }, outSearchDirection);
            }
            
            // Edge check: Is origin towards AB x ABC (outside of edge AB away from the triangle)?
            if (isSameDirection(aToB.cross(abc), aToOrigin)) {
                // Possibly cases 1 (A), 2 (B), or 4 (away from AB between A and B). Throwing out C from points for sure since origin not around there
                return onLineSimplexCheck(points = { a, b }, outSearchDirection);
            }
            
            // Edge check: Is origin towards BC x ABC (outside of edge BC away from the triangle)?
            FPVector bToC = (c - b).normalized();
            FPVector bToOrigin = -b.normalized(); // Need either bToOrigin or CcoOrigin to compare where origin is exactly in comparison to BC edge
            if (isSameDirection(bToC.cross(abc), bToOrigin)) {
                // TEMP: Observation if hitting these cases that simplified GJK algo said we could safely skip
                logger.logWarnMessage(
                    "ComplexCollisions::onTriangleSimplexCheck",
                    "Reached supposedly unnecessary triangle case"
                );
                
                // Possibly cases 2 (B), 3 (C), or 6 (away from BC between B and C). Throwing A C from points for sure since origin not around there
                return onLineSimplexCheck(points = { b, c }, outSearchDirection);
                
            }

            // Otherwise, origin is either above or below our triangle. Figure out which direction exactly and keep the triangle!
            if (isSameDirection(abc, aToOrigin)) { // Is the origin "above" the triangle?
                outSearchDirection = abc;
            }
            // Otherwise finally one case left: The origin is "below" the triangle
            else {
                points = { a, c, b }; // "Re-wind" the triangle so it points towards the new direction // TODO: Is this even necessary? Does B vs C order matter at all?
                outSearchDirection = -abc;
            }

            return false; // Cannot confirm collision as in 3D we can't confirm if origin is contained within simplex with just a triangle
        }

        bool onTetrahedronSimplexCheck(Simplex& points, FPVector& outSearchDirection) {
            // https://youtu.be/MDusDn8oTSE?t=403
            FPVector a = points[0];
            FPVector b = points[1];
            FPVector c = points[2];
            FPVector d = points[3];

            FPVector aToB = (b - a).normalized();
            FPVector aToC = (c - a).normalized();
            FPVector aToD = (d - a).normalized();
            FPVector aToOrigin = (-a).normalized();

            FPVector abc = aToB.cross(aToC).normalized(); // Outwards direction perpendicular to ABC face
            FPVector acd = aToC.cross(aToD).normalized(); // Outwards direction perpendicular to ACD face
            FPVector adb = aToD.cross(aToB).normalized(); // Outwards direction perpendicular to ADB face

            // From my current understanding, this is checking if the origin is outside three faces of our tetrahedron
            //  (and somehow we already know that the origin isn't outside the face we're not checking)
            // If the origin ISN'T outside any of the faces, then it must be within the tetrahedron- so we'd be done!
            if (isSameDirection(abc, aToOrigin)) {
                return onTriangleSimplexCheck(points = { a, b, c }, outSearchDirection);
            }
            if (isSameDirection(acd, aToOrigin)) {
                return onTriangleSimplexCheck(points = { a, c, d }, outSearchDirection);
            }
            if (isSameDirection(adb, aToOrigin)) {
                return onTriangleSimplexCheck(points = { a, d, b }, outSearchDirection);
            }

            // TODO: Calculate outwards direction perpendicular to last BDC face
            FPVector bToD = (d - b).normalized();
            FPVector bToC = (c - b).normalized();
            FPVector bToOrigin = (-b).normalized(); // Need to check if origin is beyond BCD/BDC plane
            FPVector bdc = bToD.cross(bToC).normalized();
            // TODO: Apparently the simplified GJK algo doesn't actually work. So check last face
            if (isSameDirection(bdc, bToOrigin)) {
                // TEMP: Observation if hitting these cases that simplified GJK algo said we could safely skip
                logger.logWarnMessage(
                    "ComplexCollisions::onTetrahedronSimplexCheck",
                    "Reached supposedly unnecessary tetrahedron case"
                );
                
                // If origin is somehow not in direction of A, then throw it out and go back to using B as latest point
                // TODO: Should the new direction actually be bToOrigin? Or are we trying to add the furthest *perpendicular* point to simplex on purpose?
                return onTriangleSimplexCheck(points = { b, c, d }, outSearchDirection);
            }

            return true; // Otherwise we confirmed that the origin is contained within our tetrahedron, so collision confirmed!
        }

#pragma endregion
#pragma region EPA Internal

        /// <summary>
        /// Uses EPA algorithm to compute detailed collision information after GJK confirms collision occurred.
        /// Based on the following: https://youtu.be/0XQ2FSz3EK8
        /// </summary>
        ImpactResult computeCollisionInfoViaEPA(const Collider& colliderA,
                                                const Collider& colliderB,
                                                const Simplex& gjkResultSimplex) {
            // TODO: Understand what the HECK is going on and comment thoroughly!
            
            // Store data regarding polytope we'll build
            // Note that we'll need to explicitly store faces as well in order to facilitate recreating polytope on vertex modification
            std::vector<FPVector> polytope(gjkResultSimplex.begin(), gjkResultSimplex.end());
            std::vector<size_t> faces = { // Treating every 3 indices as a triangle
                0, 1, 2,
                0, 3, 1,
                0, 2, 3,
                1, 3, 2
            };

            // list: vector4(normal, distance), index: min distance
            auto [normals, minFace] = getFaceNormals(polytope, faces);

            FPVector minNormal;
            fp minDistance = FPMath::maxLimit();

            while (minDistance == FPMath::maxLimit()) {
                minNormal = normals[minFace].v;
                minDistance = normals[minFace].w;

                FPVector support = getSupportPoint(colliderA, colliderB, minNormal);
                fp sDistance = minNormal.dot(support);

                if (FPMath::abs(sDistance - minDistance) > fp(0.001f)) {
                    minDistance = FPMath::maxLimit();

                    std::vector<std::pair<size_t, size_t>> uniqueEdges;

                    for (size_t i = 0; i < normals.size(); i++) {
                        if (isSameDirection(normals[i].v, support)) {
                            size_t f = i * 3;

                            AddIfUniqueEdge(uniqueEdges, faces, f,     f + 1);
                            AddIfUniqueEdge(uniqueEdges, faces, f + 1, f + 2);
                            AddIfUniqueEdge(uniqueEdges, faces, f + 2, f    );

                            faces[f + 2] = faces.back(); faces.pop_back();
                            faces[f + 1] = faces.back(); faces.pop_back();
                            faces[f    ] = faces.back(); faces.pop_back();

                            normals[i] = normals.back(); normals.pop_back();

                            i--;
                        }
                    }

                    if (uniqueEdges.size() == 0) {
                        break;
                    }

                    std::vector<size_t> newFaces;
                    for (auto [edge1, edge2] : uniqueEdges) {
                        newFaces.push_back(edge1);
                        newFaces.push_back(edge2);
                        newFaces.push_back(polytope.size());
                    }

                    polytope.push_back(support);

                    auto [newNormals, newMinFace] = getFaceNormals(polytope, newFaces);

                    fp newMinDistance = FPMath::maxLimit();
                    for (size_t i = 0; i < normals.size(); i++) {
                        if (normals[i].w < newMinDistance) {
                            newMinDistance = normals[i].w;
                            minFace = i;
                        }
                    }

                    if (newNormals[newMinFace].w < newMinDistance) {
                        minFace = newMinFace + normals.size();
                    }

                    faces.insert(faces.end(), newFaces.begin(), newFaces.end());
                    normals.insert(normals.end(), newNormals.begin(), newNormals.end());
                }
            }

            if (minDistance == FPMath::maxLimit()) {
                logger.logWarnMessage(
                    "ComplexCollisions::computeCollisionInfoViaEPA",
                    "Did not find collision in EPA algorithm!"
                );
                return ImpactResult::noCollision();
            }

            // TODO: Return penetration depth scalar and direction separately
            fp penDepth = minDistance + fp{0.001f};
            ImpactResult impactResult(minNormal, penDepth);
            return impactResult;
        }

        std::pair<std::vector<FPVector4>, size_t> getFaceNormals(const std::vector<FPVector>& polytope,
                                                                        const std::vector<size_t>& faces) {
            std::vector<FPVector4> normals;
            size_t minTriangle = 0;
            fp minDistance = FPMath::maxLimit();

            for (size_t i = 0; i < faces.size(); i += 3) {
                FPVector a = polytope[faces[i    ]];
                FPVector b = polytope[faces[i + 1]];
                FPVector c = polytope[faces[i + 2]];

                FPVector normal = (b - a).cross(c - a).normalized();
                fp distance = normal.dot(a);

                if (distance < fp{0}) {
                    normal = normal * fp{-1};
                    distance *= fp{-1};
                }

                normals.emplace_back(FPVector4(distance, normal));

                if (distance < minDistance) {
                    minTriangle = i / 3;
                    minDistance = distance;
                }
            }

            return { normals, minTriangle };
        }

        void AddIfUniqueEdge(std::vector<std::pair<size_t, size_t>>& edges,
                                  const std::vector<size_t>& faces,
                                  const size_t a,
                                  const size_t b) {
            // TODO: ???
            auto reverse = std::find(edges.begin(),edges.end(),std::make_pair(faces[b], faces[a]));

            if (reverse != edges.end()) {
                edges.erase(reverse);
            }
            else {
                edges.emplace_back(faces[a], faces[b]);
            }
        }
        
#pragma endregion 
        
    };
}
