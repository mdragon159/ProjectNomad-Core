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
        // TODO: What happens if no collision? (Bad user input). Or does it even matter with how calculations are done?
        
        ImpactResult isColliding(const Collider& A, const Collider& B) {
            if (A.isNotInitialized()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isColliding",
                    "Collider A was not initialized type"
                );
                return ImpactResult::noCollision();
            }
            if (B.isNotInitialized()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isColliding",
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
                    return isCapsuleAndBoxColliding(A, B);
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
                    return isSphereAndBoxColliding(A, B);
                }
                if (B.isCapsule()) {
                    return isSphereAndCapsuleColliding(A, B);
                }
                if (B.isSphere()) {
                    return isSphereAndSphereColliding(A, B);
                }
            }

            logger.logErrorMessage(
                "CollisionsSimple::isColliding",
                "Did not find a matching function for colliders A and B of types: "
                            + A.getTypeAsString() + ", " + B.getTypeAsString() 
            );
            return ImpactResult::noCollision();   
        }

        ImpactResult isBoxAndBoxColliding(const Collider& boxA, const Collider& boxB) {
            if (!boxA.isBox()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isBoxAndBoxColliding",
                    "Collider A was not a box but instead a " + boxA.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!boxB.isBox()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isBoxAndBoxColliding",
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
                if (!simpleCollisions.isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, bNormal, smallestPenDepth,
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

            // We could find no separating axis, so definitely intersecting
            return ImpactResult(smallestPenDepth * penDepthAxis);
        }

        ImpactResult isCapsuleAndCapsuleColliding(const Collider& capA, const Collider& capB) {
            if (!capA.isCapsule()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isCapsuleAndCapsuleColliding",
                    "Collider A was not a capsule but instead a " + capA.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!capB.isCapsule()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isCapsuleAndCapsuleColliding",
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
            fp s, t;
            FPVector c1, c2;
            fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
                aLinePoints.start, aLinePoints.end,
                bLinePoints.start, bLinePoints.end,
                s, t, c1, c2
            );

            // If (squared) distance smaller than (squared) sum of radii, they collide
            fp radius = capA.getCapsuleRadius() + capB.getCapsuleRadius();
            bool isColliding = distSquared < radius * radius;

            // TODO: Calculate penetration info and such
            return isColliding ? ImpactResult(FPVector::zero()) : ImpactResult::noCollision();
        }

        ImpactResult isSphereAndSphereColliding(const Collider& sphereA, const Collider& sphereB) {
            if (!sphereA.isSphere()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isSphereAndSphereColliding",
                    "Collider A was not a sphere but instead a " + sphereA.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!sphereB.isSphere()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isSphereAndSphereColliding",
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
                FPVector intersectionDepthAndDirection = centerDifference.normalized() * intersectionDepth;
                return ImpactResult(intersectionDepthAndDirection);
            }
            
            return ImpactResult::noCollision();
        }

        ImpactResult isBoxAndCapsuleColliding(const Collider& box, const Collider& capsule) {
            if (!box.isBox()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isBoxAndCapsuleColliding",
                    "Collider box was not a box but instead a " + box.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!capsule.isCapsule()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isBoxAndCapsuleColliding",
                    "Collider capsule was not a capsule but instead a " + capsule.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }

            // TODO: Document base logic from Real-Time Collisions 5.5.7. Doc should be similar to other methods
            // TODO: Also rename vars

            // Compute line points for capsule
            auto worldSpaceCapsulePoints = capsule.getCapsuleMedialLineExtremes();

            // Convert line points from world space to local space so can continue with Capsule vs AABB approach
            FPVector boxSpaceCapsulePointA = box.toLocalSpaceFromWorld(worldSpaceCapsulePoints.start);
            FPVector boxSpaceCapsulePointB = box.toLocalSpaceFromWorld(worldSpaceCapsulePoints.end);

            // Compute the AABB resulting from expanding box faces by capsule radius
            Collider checkAgainstBox(box);
            checkAgainstBox.setCenter(FPVector::zero());  // Doing everything in box local space, so center should be at origin
            checkAgainstBox.setRotation(FPQuat::identity()); // Unnecessary but like being clear that this is an AABB (no rotation)
            checkAgainstBox.setBoxHalfSize(box.getBoxHalfSize() + FPVector(capsule.getCapsuleRadius()));

            // Edge case: Raycast (next step) implementation doesn't check if starts in box
            // Thus, check if beginning point is within box. If so, capsule is located within box and thus collision
            //      Note that we don't need to check other end since raycast starts from this point; if second point is
            //      located within the box, then raycast will return an intersection
            if (checkAgainstBox.isLocalSpacePtWithinBox(boxSpaceCapsulePointA)) {
                return ImpactResult(FPVector::zero()); // TODO: Pen depth/impact info
            }
            
            // Intersect ray against expanded box. Exit with no intersection if ray misses box, else get intersection point and time as result
            // NOTE: Pretty certain didn't need to convert to box space for this test (since raycast does that)
            //          Minor optimization: Do this raycast test *before* converting to box local space
            //          HOWEVER, may not work due to following math (and if not possible, then update these comments so don't go down this path again)
            fp timeOfIntersection;
            FPVector intersectionPoint;
            Ray intersectionTestRay = Ray::fromPoints(boxSpaceCapsulePointA, boxSpaceCapsulePointB);
            bool didRaycastIntersectCheckBox = simpleCollisions.raycastWithBox(intersectionTestRay, checkAgainstBox,
                                                    timeOfIntersection, intersectionPoint);
            if (!didRaycastIntersectCheckBox) { // If raycast actually hit...
                return ImpactResult::noCollision();
            }
            // If intersection greater than medial line length, then no intersection for sure
            if (timeOfIntersection > capsule.getMedialHalfLineLength()) {
                return ImpactResult::noCollision();
            }

            // Convert check against box to min and max extents, as original algorithm is based on that representation
            FPVector minBoxExtents = -checkAgainstBox.getBoxHalfSize();
            FPVector maxBoxExtents = checkAgainstBox.getBoxHalfSize();

            // Compute which min and max faces of box the intersection point lies outside of
            // Note, u and v cannot have the same bits set and they must have at least one bit set among them
            uint32_t u = 0, v = 0;
            if (intersectionPoint.x < minBoxExtents.x) u |= 1;
            if (intersectionPoint.x > maxBoxExtents.x) v |= 1;
            if (intersectionPoint.y < minBoxExtents.y) u |= 2;
            if (intersectionPoint.y > maxBoxExtents.y) v |= 2;
            if (intersectionPoint.z < minBoxExtents.z) u |= 4;
            if (intersectionPoint.z > maxBoxExtents.z) v |= 4;

            // "Or" all set bits together into a bit mask (note: here u + v == u | v)
            uint32_t m = u + v;

            // Define line segment representing capsule's medial line segment, in obb space
            Line boxSpaceCapsuleMedialSegment(boxSpaceCapsulePointA, boxSpaceCapsulePointB);

            // If all 3 bits set (m == 7) then intersection point is in a vertex region
            if (m == 7) {
                // Must now intersect capsule line segment against the capsules of the three
                // edges meeting at the vertex and return the best time, if one or more hit
                fp tMin = FPMath::maxLimit();

                FPVector unused;
                Line testCapsuleMedianLine(simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, v), unused);

                testCapsuleMedianLine.end = simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, v ^ 1);
                bool didIntersect = simpleCollisions.linetestWithCapsule(boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                                                capsule.getCapsuleRadius(), timeOfIntersection, unused);
                if (didIntersect) tMin = FPMath::min(timeOfIntersection, tMin);

                testCapsuleMedianLine.end = simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, v ^ 2);
                didIntersect = simpleCollisions.linetestWithCapsule(boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                                            capsule.getCapsuleRadius(), timeOfIntersection, unused);
                if (didIntersect) tMin = FPMath::min(timeOfIntersection, tMin);

                testCapsuleMedianLine.end = simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, v ^ 4);
                didIntersect = simpleCollisions.linetestWithCapsule(boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                                            capsule.getCapsuleRadius(), timeOfIntersection, unused);
                if (didIntersect) tMin = FPMath::min(timeOfIntersection, tMin);

                if (tMin == fp{FLT_MAX}) return ImpactResult::noCollision(); // No intersection
                
                timeOfIntersection = tMin;

                // TODO: Pen depth/impact info
                return ImpactResult(FPVector::zero()); // Intersection at time t == tmin /
            }

            // TODO: How to adjust below code to NOT count simply touching as collision?
            
            // If only one bit set in m, then intersection point is in a face region
            if ((m & (m - 1)) == 0) {
                // Do nothing. Time t from intersection with expanded box is correct intersection time
                return ImpactResult(FPVector::zero()); // TODO: Pen depth/impact info
            }
            
            // p is in an edge region. Intersect against the capsule at the edge
            Line testCapsuleMedianLine(simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, u ^ 7), simpleCollisions.getCorner(minBoxExtents, maxBoxExtents, v));
            bool didIntersect = simpleCollisions.linetestWithCapsule(
                boxSpaceCapsuleMedialSegment,
                testCapsuleMedianLine,
                capsule.getCapsuleRadius(),
                timeOfIntersection,
                intersectionPoint
            );
            
            return didIntersect ? ImpactResult(FPVector::zero()) : ImpactResult::noCollision();
        }

        ImpactResult isBoxAndSphereColliding(const Collider& box, const Collider& sphere) {
            if (!box.isBox()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isBoxAndSphereColliding",
                    "Collider box was not a box but instead a " + box.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!sphere.isSphere()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isBoxAndSphereColliding",
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
            if (sphereCenterToBoxDistance == fp{0}) {
                // Sphere center within box case
                // TODO: One day calculate distance to nearest face for penetration depth!
                return ImpactResult(FPVector::zero());
            }

            fp intersectionDepth = sphere.getSphereRadius() - sphereCenterToBoxDistance;
            if (intersectionDepth > fp{0}) {
                FPVector intersectionDepthAndDirection = closestPointOffSetToSphere.normalized() * intersectionDepth;
                return ImpactResult(intersectionDepthAndDirection);
            }

            return ImpactResult::noCollision();
        }

        ImpactResult isCapsuleAndSphereColliding(const Collider& capsule, const Collider& sphere) {
            if (!capsule.isCapsule()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isCapsuleAndSphereColliding",
                    "Collider capsule was not a capsule but instead a " + capsule.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }
            if (!sphere.isSphere()) {
                logger.logErrorMessage(
                    "CollisionsSimple::isCapsuleAndSphereColliding",
                    "Collider sphere was not a sphere but instead a " + sphere.getTypeAsString()
                );
                return ImpactResult::noCollision();
            }

            // TODO: Document base logic from Real-Time Collisions 4.5.1 "TestSphereCapsule" method. Doc should be similar to other methods
            // TODO: Also rename vars

            // Get median line of capsule as points
            auto capsulePoints = capsule.getCapsuleMedialLineExtremes();

            // Compute (squared) distance between sphere center and capsule line segment
            fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(capsulePoints, sphere.getCenter());
            
            // If (squared) distance smaller than (squared) sum of radii, they collide
            fp radius = sphere.getSphereRadius() + capsule.getCapsuleRadius();
            bool isColliding = distSquared < radius * radius;
            
            return isColliding ? ImpactResult(FPVector::zero()) : ImpactResult::noCollision();
        }

        // Extras below for ease of typing. Mostly due to ease of scanning for consistency in isColliding
        ImpactResult isCapsuleAndBoxColliding(const Collider& capsule, const Collider& box) {
            return isBoxAndCapsuleColliding(box, capsule);
        }
        ImpactResult isSphereAndBoxColliding(const Collider& sphere, const Collider& box) {
            return isBoxAndSphereColliding(box, sphere);
        }
        ImpactResult isSphereAndCapsuleColliding(const Collider& sphere, const Collider& capsule) {
            return isCapsuleAndSphereColliding(capsule, sphere);
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
            ImpactResult impactResult(minNormal * minDistance);
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
