#include "SimpleCollisions.h"

#include "Context/CoreContext.h"
#include "CollisionHelpers.h"
#include "Model/Line.h"
#include "Model/Ray.h"
#include "Model/FCollider.h"
#include "Math/FPMath.h"

namespace ProjectNomad {
    bool SimpleCollisions::IsColliding(CoreContext& coreContext, const FCollider& A, const FCollider& B) {
        if (A.IsNotInitialized()) {
            coreContext.logger.LogErrorMessage("Collider A was not initialized type");
            return false;
        }
        if (B.IsNotInitialized()) {
            coreContext.logger.LogErrorMessage("Collider B was not initialized type");
            return false;
        }

        if (A.IsBox()) {
            if (B.IsBox()) {
                return IsBoxAndBoxColliding(coreContext, A, B);
            }
            if (B.IsCapsule()) {
                return IsBoxAndCapsuleColliding(coreContext, A, B);
            }
            if (B.IsSphere()) {
                return IsBoxAndSphereColliding(coreContext, A, B);
            }
        }
        if (A.IsCapsule()) {
            if (B.IsBox()) {
                return IsCapsuleAndBoxColliding(coreContext, A, B);
            }
            if (B.IsCapsule()) {
                return IsCapsuleAndCapsuleColliding(coreContext, A, B);
            }
            if (B.IsSphere()) {
                return IsCapsuleAndSphereColliding(coreContext, A, B);
            }
        }
        if (A.IsSphere()) {
            if (B.IsBox()) {
                return IsSphereAndBoxColliding(coreContext, A, B);
            }
            if (B.IsCapsule()) {
                return IsSphereAndCapsuleColliding(coreContext, A, B);
            }
            if (B.IsSphere()) {
                return IsSphereAndSphereColliding(coreContext, A, B);
            }
        }

        coreContext.logger.LogErrorMessage(
            "Did not find a matching function for colliders A and B of types: "
            + A.GetTypeAsString() + ", " + B.GetTypeAsString()
        );
        return false;
    }

    bool SimpleCollisions::IsBoxAndBoxColliding(CoreContext& coreContext, const FCollider& boxA, const FCollider& boxB) {
        if (!boxA.IsBox()) {
            coreContext.logger.LogErrorMessage("Collider A was not a box but instead a " + boxA.GetTypeAsString());
            return false;
        }
        if (!boxB.IsBox()) {
            coreContext.logger.LogErrorMessage("Collider B was not a box but instead a " + boxA.GetTypeAsString());
            return false;
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
        // TODO: Use the Real-Time Collision Detection algorithm, which is far more efficient but still a bit over my head

        // Create necessary data used for SAT tests
        std::vector<FVectorFP> aNormals = boxA.GetBoxNormalsInWorldCoordinates();
        std::vector<FVectorFP> bNormals = boxB.GetBoxNormalsInWorldCoordinates();
        std::vector<FVectorFP> aVertices = boxA.GetBoxVerticesInWorldCoordinates();
        std::vector<FVectorFP> bVertices = boxB.GetBoxVerticesInWorldCoordinates();

        // Estimate penetration depth by keeping track of shortest penetration axis
        fp smallestPenDepth = fp{-1};
        FVectorFP penDepthAxis;

        // Check for separating axis with a and b normals
        for (const FVectorFP& aNormal : aNormals) {
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, aNormal, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }
        }
        for (const FVectorFP& bNormal : bNormals) {
            if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, bNormal, smallestPenDepth,
                                                              penDepthAxis)) {
                return false;
            }
        }

        // Now check for axes based on cross products with each combination of normals
        // FUTURE: Probs could do the deal-with-square-at-end trick instead of full normalizations
        FVectorFP testAxis;
        testAxis = aNormals.at(0).Cross(bNormals.at(0)).Normalized(); // Normalize to fix pen depth calculations
        if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                          penDepthAxis)) {
            return false;
        }

        testAxis = aNormals.at(0).Cross(bNormals.at(1)).Normalized();
        if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                          penDepthAxis)) {
            return false;
        }

        testAxis = aNormals.at(0).Cross(bNormals.at(2)).Normalized();
        if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                          penDepthAxis)) {
            return false;
        }

        testAxis = aNormals.at(1).Cross(bNormals.at(0)).Normalized();
        if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                          penDepthAxis)) {
            return false;
        }

        testAxis = aNormals.at(1).Cross(bNormals.at(1)).Normalized();
        if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                          penDepthAxis)) {
            return false;
        }

        testAxis = aNormals.at(1).Cross(bNormals.at(2)).Normalized();
        if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                          penDepthAxis)) {
            return false;
        }

        testAxis = aNormals.at(2).Cross(bNormals.at(0)).Normalized();
        if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                          penDepthAxis)) {
            return false;
        }

        testAxis = aNormals.at(2).Cross(bNormals.at(1)).Normalized();
        if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                          penDepthAxis)) {
            return false;
        }

        testAxis = aNormals.at(2).Cross(bNormals.at(2)).Normalized();
        if (!isIntersectingAlongAxisAndUpdatePenDepthVars(aVertices, bVertices, testAxis, smallestPenDepth,
                                                          penDepthAxis)) {
            return false;
        }

        // We could find no separating axis, so definitely intersecting
        return true;
    }

    bool SimpleCollisions::IsCapsuleAndCapsuleColliding(CoreContext& coreContext, const FCollider& capA, const FCollider& capB) {
        if (!capA.IsCapsule()) {
            coreContext.logger.LogErrorMessage("Collider A was not a capsule but instead a " + capA.GetTypeAsString());
            return false;
        }
        if (!capB.IsCapsule()) {
            coreContext.logger.LogErrorMessage("Collider B was not a capsule but instead a " + capB.GetTypeAsString());
            return false;
        }

        // TODO: Document base logic from Real-Time Collisions 4.5.1 "TestCapsuleCapsule" method. Doc should be similar to other methods
        // TODO: Also rename vars

        // Get median line of capsules as points
        auto aLinePoints = capA.GetCapsuleMedialLineExtremes();
        auto bLinePoints = capB.GetCapsuleMedialLineExtremes();

        // Compute distance between the median line segments
        // "Compute (squared) distance between the inner structures of the capsules" (from book)
        fp s, t;
        FVectorFP c1, c2;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            aLinePoints.start, aLinePoints.end,
            bLinePoints.start, bLinePoints.end,
            s, t, c1, c2
        );

        // If (squared) distance smaller than (squared) sum of radii, they collide
        fp radius = capA.GetCapsuleRadius() + capB.GetCapsuleRadius();
        bool isColliding = distSquared < radius * radius;

        return isColliding;
    }

    bool SimpleCollisions::IsSphereAndSphereColliding(CoreContext& coreContext, const FCollider& sphereA,
                                           const FCollider& sphereB) {
        if (!sphereA.IsSphere()) {
            coreContext.logger.
                        LogErrorMessage("Collider A was not a sphere but instead a " + sphereA.GetTypeAsString());
            return false;
        }
        if (!sphereB.IsSphere()) {
            coreContext.logger.
                        LogErrorMessage("Collider B was not a sphere but instead a " + sphereB.GetTypeAsString());
            return false;
        }

        // Solid explanation of core algorithm with pictures here:
        // https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection#bounding_spheres

        FVectorFP centerDifference = sphereB.GetCenter() - sphereA.GetCenter();
        fp centerDistance = centerDifference.GetLength();
        fp intersectionDepth = (sphereA.GetSphereRadius() + sphereB.GetSphereRadius()) - centerDistance;

        return intersectionDepth > fp{0};
    }

    bool SimpleCollisions::IsBoxAndCapsuleColliding(CoreContext& coreContext, const FCollider& box, const FCollider& capsule) {
        if (!box.IsBox()) {
            coreContext.logger.LogErrorMessage("Collider box was not a box but instead a " + box.GetTypeAsString());
            return false;
        }
        if (!capsule.IsCapsule()) {
            coreContext.logger.LogErrorMessage(
                "Collider capsule was not a capsule but instead a " + capsule.GetTypeAsString());
            return false;
        }

        // Base logic is from Real-Time Collisions 5.5.7

        // Compute line points for capsule
        auto worldSpaceCapsulePoints = capsule.GetCapsuleMedialLineExtremes();
        // Convert line points from world space to local space so can continue with Capsule vs AABB approach
        FVectorFP boxSpaceCapsulePointA = box.ToLocalSpaceFromWorld(worldSpaceCapsulePoints.start);
        FVectorFP boxSpaceCapsulePointB = box.ToLocalSpaceFromWorld(worldSpaceCapsulePoints.end);
        Line boxSpaceCapsuleMedialSegment(boxSpaceCapsulePointA, boxSpaceCapsulePointB);

        // Compute the AABB resulting from expanding box faces by capsule radius
        FCollider checkAgainstBox(box);
        checkAgainstBox.SetBoxHalfSize(box.GetBoxHalfSize() + FVectorFP(capsule.GetCapsuleRadius()));

        // Intersect ray against expanded box. Exit with no intersection if ray misses box, else get intersection point and time as result
        //          Side note: Pretty certain didn't need to convert to box space for this test (since raycast does that)
        //          Future minor optimization: Do this raycast test *before* converting to box local space
        //          HOWEVER, may not work due to following math (and if not possible, then update these comments so don't go down this path again)
        fp timeOfIntersection;
        FVectorFP intersectionPoint;
        Ray intersectionTestRay = Ray::fromPoints(boxSpaceCapsulePointA, boxSpaceCapsulePointB);
        bool didRaycastIntersectCheckBox = raycastForAABB(coreContext, intersectionTestRay, checkAgainstBox,
                                                                 timeOfIntersection, intersectionPoint);
        // If raycast did not hit at all then definitely no collision
        if (!didRaycastIntersectCheckBox) {
            return false;
        }
        // Verify that raycast intersection point is within range of the capsule median line
        // (ie, turn this into a linetest)
        if (timeOfIntersection >= capsule.GetMedialHalfLineLength() * 2) {
            // Edge case: Median line is within the expanded box but doesn't intersect with the surface of the box.
            //              Raycast usage will not catch this case so check for it explicitly 
            if (checkAgainstBox.IsLocalSpacePtWithinBoxExcludingOnSurface(boxSpaceCapsulePointA)) {
                // Default to final capsule median line point for further calculations
                // Side note: Not sure whether to pick initial or final point, but given there's a min operation
                //              operation with time later on, it's safest to use the maximum time. No idea if matters
                timeOfIntersection = fp{1};
                // Latter linetest considers 1 = 100% of line length. Yes this is inconsistent with raycast
                intersectionPoint = boxSpaceCapsulePointB;
            }
            // Otherwise the box is certainly too far for an intersection
            else {
                return false;
            }
        }

        // Compute which min and max faces of box the intersection point lies outside of.
        // Note, u and v cannot have the same bits set and they must have at least one bit set among them
        uint32_t lessThanMinExtentChecks = 0, greaterThanMaxExtentChecks = 0;
        FVectorFP maxBoxExtents = box.GetBoxHalfSize();
        FVectorFP minBoxExtents = -maxBoxExtents;
        if (intersectionPoint.x < minBoxExtents.x) lessThanMinExtentChecks |= 1;
        if (intersectionPoint.x > maxBoxExtents.x) greaterThanMaxExtentChecks |= 1;
        if (intersectionPoint.y < minBoxExtents.y) lessThanMinExtentChecks |= 2;
        if (intersectionPoint.y > maxBoxExtents.y) greaterThanMaxExtentChecks |= 2;
        if (intersectionPoint.z < minBoxExtents.z) lessThanMinExtentChecks |= 4;
        if (intersectionPoint.z > maxBoxExtents.z) greaterThanMaxExtentChecks |= 4;

        // "Or" all set bits together into a bit mask (note: effectively here u + v == u | v as same bit can't be set in both variables)
        uint32_t mask = lessThanMinExtentChecks + greaterThanMaxExtentChecks;

        // If all 3 bits set (m == 7) then intersection point (if any) is in a vertex region
        if (mask == 7) {
            bool didIntersect;
            FVectorFP unused;

            // Must now intersect capsule line segment against the capsules of the three
            // edges meeting at the vertex and return the best time, if one or more hit
            fp tMin = FPMath::maxLimit();

            // Note that endpoint of test line will be changed for each test
            Line testCapsuleMedianLine(getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks), unused);

            testCapsuleMedianLine.end = getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks ^ 1);
            didIntersect = LinetestWithCapsule(coreContext, boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                                               capsule.GetCapsuleRadius(), timeOfIntersection, unused);
            if (didIntersect) {
                tMin = FPMath::min(timeOfIntersection, tMin);
            }

            testCapsuleMedianLine.end = getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks ^ 2);
            didIntersect = LinetestWithCapsule(coreContext, boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                                               capsule.GetCapsuleRadius(), timeOfIntersection, unused);
            if (didIntersect) {
                tMin = FPMath::min(timeOfIntersection, tMin);
            }

            testCapsuleMedianLine.end = getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks ^ 4);
            didIntersect = LinetestWithCapsule(coreContext, boxSpaceCapsuleMedialSegment, testCapsuleMedianLine,
                                               capsule.GetCapsuleRadius(), timeOfIntersection, unused);
            if (didIntersect) {
                tMin = FPMath::min(timeOfIntersection, tMin);
            }

            // If didn't find a single intersection, then confirmed that there was no intersection
            if (tMin == FPMath::maxLimit()) {
                return false;
            }

            timeOfIntersection = tMin;
            return true; // Intersection at time t == tmin
        }

        // If only one bit set in m, then intersection point is in a face region
        if ((mask & (mask - 1)) == 0) {
            // Do nothing. Time t from intersection with expanded box is correct intersection time
            return true;
        }

        // p is in an edge region. Intersect against the capsule at the edge
        Line testCapsuleMedianLine(
            getCorner(minBoxExtents, maxBoxExtents, lessThanMinExtentChecks ^ 7),
            getCorner(minBoxExtents, maxBoxExtents, greaterThanMaxExtentChecks)
        );
        bool didIntersect = LinetestWithCapsule(
            coreContext,
            boxSpaceCapsuleMedialSegment,
            testCapsuleMedianLine,
            capsule.GetCapsuleRadius(),
            timeOfIntersection,
            intersectionPoint
        );

        return didIntersect;
    }

    bool SimpleCollisions::IsBoxAndSphereColliding(CoreContext& coreContext, const FCollider& box, const FCollider& sphere) {
        if (!box.IsBox()) {
            coreContext.logger.LogErrorMessage("Collider box was not a box but instead a " + box.GetTypeAsString());
            return false;
        }
        if (!sphere.IsSphere()) {
            coreContext.logger.LogErrorMessage(
                "Collider sphere was not a sphere but instead a " + sphere.GetTypeAsString());
            return false;
        }

        // Algorithm references:
        // AABB vs Sphere https://developer.mozilla.org/en-US/docs/Games/Techniques/3D_collision_detection#bounding_spheres
        // OOBB vs Sphere https://gamedev.stackexchange.com/a/163874/67844

        // Convert sphere's center to OBB's local space, with OOB's center as origin
        // This allows us to use simplified Sphere vs AABB logic here on out
        FVectorFP localSphereCenter = box.ToLocalSpaceFromWorld(sphere.GetCenter());

        // Find the closest point on the box to the sphere's center
        // Essentially just uses sphere center coordinate on each axis IF within box, otherwise clamps to nearest box extent
        // Suggest visualizing with number lines
        const FVectorFP& extents = box.GetBoxHalfSize();
        // Represents min and max points, since using box center as origin
        FVectorFP closestBoxPointToSphere;
        closestBoxPointToSphere.x = FPMath::max(-extents.x, FPMath::min(localSphereCenter.x, extents.x));
        closestBoxPointToSphere.y = FPMath::max(-extents.y, FPMath::min(localSphereCenter.y, extents.y));
        closestBoxPointToSphere.z = FPMath::max(-extents.z, FPMath::min(localSphereCenter.z, extents.z));

        // Finally do ordinary point distance vs sphere radius checking
        FVectorFP closestPointOffSetToSphere = localSphereCenter - closestBoxPointToSphere;
        fp sphereCenterToBoxDistance = closestPointOffSetToSphere.GetLength();
        if (sphereCenterToBoxDistance == fp{0}) {
            // Sphere center within box case
            return true;
        }

        fp intersectionDepth = sphere.GetSphereRadius() - sphereCenterToBoxDistance;
        return intersectionDepth > fp{0};
    }

    bool SimpleCollisions::IsCapsuleAndSphereColliding(CoreContext& coreContext, const FCollider& capsule,
                                            const FCollider& sphere) {
        if (!capsule.IsCapsule()) {
            coreContext.logger.LogErrorMessage(
                "Collider capsule was not a capsule but instead a " + capsule.GetTypeAsString());
            return false;
        }
        if (!sphere.IsSphere()) {
            coreContext.logger.LogErrorMessage(
                "Collider sphere was not a sphere but instead a " + sphere.GetTypeAsString());
            return false;
        }

        // TODO: Document base logic from Real-Time Collisions 4.5.1 "TestSphereCapsule" method. Doc should be similar to other methods
        // TODO: Also rename vars

        // Get median line of capsule as points
        auto capsulePoints = capsule.GetCapsuleMedialLineExtremes();

        // Compute (squared) distance between sphere center and capsule line segment
        fp distSquared = CollisionHelpers::getSquaredDistBetweenPtAndSegment(capsulePoints, sphere.GetCenter());

        // If (squared) distance smaller than (squared) sum of radii, they collide
        fp radius = sphere.GetSphereRadius() + capsule.GetCapsuleRadius();
        bool isColliding = distSquared < radius * radius;

        return isColliding;
    }

    // Extras below for ease of typing. Mostly due to ease of scanning for consistency in isColliding
    bool SimpleCollisions::IsCapsuleAndBoxColliding(CoreContext& coreContext, const FCollider& capsule, const FCollider& box) {
        return IsBoxAndCapsuleColliding(coreContext, box, capsule);
    }

    bool SimpleCollisions::IsSphereAndBoxColliding(CoreContext& coreContext, const FCollider& sphere, const FCollider& box) {
        return IsBoxAndSphereColliding(coreContext, box, sphere);
    }

    bool SimpleCollisions::IsSphereAndCapsuleColliding(CoreContext& coreContext, const FCollider& sphere,
                                            const FCollider& capsule) {
        return IsCapsuleAndSphereColliding(coreContext, capsule, sphere);
    }


    /// <summary>
    /// Check if and when a ray and sphere intersect
    /// Based on Game Physics Cookbook, Ch 7
    /// </summary>
    /// <param name="ray">Ray to test with</param>
    /// <param name="sphere">Sphere to test with</param>
    /// <param name="timeOfIntersection">If intersection occurs, this will be time which ray first intersects with sphere</param>
    /// <param name="pointOfIntersection">Location where intersection occurs</param>
    /// <returns>True if intersection occurs, false otherwise</returns>
    bool SimpleCollisions::RaycastWithSphere(CoreContext& coreContext,
                                  const Ray& ray,
                                  const FCollider& sphere,
                                  fp& timeOfIntersection,
                                  FVectorFP& pointOfIntersection) {
        if (!sphere.IsSphere()) {
            coreContext.logger.LogErrorMessage(
                "CollisionsSimple::raycast",
                "Provided collider was not a sphere but instead a " + sphere.GetTypeAsString()
            );
            return false;
        }

        // TODO: Re-read book's "How it works" section, add more personal notes, and rename vars
        // Lotta segments and need picture and such... Ahhh....

        FVectorFP originToSphereCenter = sphere.GetCenter() - ray.origin;
        fp distBetweenCentersSq = originToSphereCenter.GetLengthSquared();
        fp radiusSq = sphere.GetSphereRadius() * sphere.GetSphereRadius();

        // Project the vector pointing from the origin of the ray to the sphere onto the direction 
        //  of the ray
        // Note: Ray direction should be normalized
        fp a = originToSphereCenter.Dot(ray.direction); // TOOD: Rename var

        // Construct the sides a triangle using the radius of the circle at the projected point 
        //  from the last step.The sides of this triangle are radius, b, and f. We work with
        //  squared units
        fp bSq = distBetweenCentersSq - a * a; // TODO: Rename vars
        fp f = FPMath::sqrt(radiusSq - bSq);

        // Compare the length of the squared radius against the hypotenuse of the triangle from 
        //  the last step

        // Case: No collision has happened
        if (radiusSq - (distBetweenCentersSq - a * a) < fp{0}) {
            return false; // -1 = no intersection
        }
        // Case: Ray starts inside the sphere
        if (distBetweenCentersSq < radiusSq) {
            timeOfIntersection = a + f;
            pointOfIntersection = ray.origin + ray.direction * timeOfIntersection;
            return true; // No need to check if time of intersection is positive or not as ray started inside sphere
        }
        // Otherwise normal intersection
        timeOfIntersection = a - f;
        pointOfIntersection = ray.origin + ray.direction * timeOfIntersection;
        return timeOfIntersection >= fp{0}; // If negative, then sphere was behind ray
    }

    /// <summary>
    /// Checks if and when a ray and OBB intersect
    /// </summary>
    /// <param name="ray">Starting point and direction to check for intersection with</param>
    /// <param name="box">Box to check for intersection with</param>
    /// <param name="timeOfIntersection">
    /// If intersection occurs, this will be time which ray first intersects with OBB.
    /// Note that if ray origin is within OBB, then this will signify when the ray hits the OBB on the way out.
    /// </param>
    /// <param name="pointOfIntersection">Location where intersection occurs</param>
    bool SimpleCollisions::RaycastWithBox(CoreContext& coreContext,
                               const Ray& ray,
                               const FCollider& box,
                               fp& timeOfIntersection,
                               FVectorFP& pointOfIntersection) {
        if (!box.IsBox()) {
            coreContext.logger.LogErrorMessage(
                "CollisionsSimple::raycast",
                "Provided collider was not a box but instead a " + box.GetTypeAsString()
            );
            return false;
        }

        // 1. Convert ray to box space
        Ray localSpaceRay(box.ToLocalSpaceFromWorld(ray.origin), box.ToLocalSpaceForOriginCenteredValue(ray.direction));

        // 2. Compute results via treating as standard AABB situation
        FVectorFP localPointOfIntersection;
        bool didIntersect = raycastForAABB(coreContext, localSpaceRay, box, timeOfIntersection,
                                                  localPointOfIntersection);

        // 3. Convert results to world space if necessary
        if (didIntersect) {
            pointOfIntersection = box.ToWorldSpaceFromLocal(localPointOfIntersection);
            // Reverse of earlier operations for localSpaceRay origin
        }

        return didIntersect;
    }

    // TODO: Raycast capsule

    /// <summary>
    /// Checks if a line and OBB intersect
    /// Originally based on Game Physics Cookbook, Chapter 10
    /// </summary>
    /// <param name="line">Line to check if intersects against box</param>
    /// <param name="box">Box to test line against</param>
    /// <param name="timeOfIntersection">
    /// If intersection occurs, this will be time which line first intersects with OBB.
    /// Note that if line origin is within OBB, then this will signify when the line hits the OBB on the way out.
    /// </param>
    /// <param name="pointOfIntersection">Location where intersection occurs</param>
    /// <returns>True if line and OBB intersect, false otherwise</returns>
    bool SimpleCollisions::LinetestWithBox(CoreContext& coreContext,
                                const Line& line,
                                const FCollider& box,
                                fp& timeOfIntersection,
                                FVectorFP& pointOfIntersection) {
        if (!box.IsBox()) {
            coreContext.logger.LogErrorMessage(
                "CollisionsSimple::raycast",
                "Provided collider was not a box but instead a " + box.GetTypeAsString()
            );
            return false;
        }

        // TODO: Check if using TestSegmentAABB from Real-Time Collision Detection 5.3.3 is more efficient

        // Reuse raycast logic for intersection testing
        Ray ray(
            line.start,
            (line.end - line.start).Normalized()
        );
        bool didRaycastHit = RaycastWithBox(coreContext, ray, box, timeOfIntersection, pointOfIntersection);

        // Return results. If raycast hit, then linetest also hits if hit isn't too far
        //  Note: t vs length comparison works as ray/line travels 1 unit along ray/line per second. 
        //        Thus, can use time as a distance metric as well
        const fp& t = timeOfIntersection; // For easier reading
        return didRaycastHit && t >= fp{0} && t * t <= line.getLengthSquared();
    }

    bool SimpleCollisions::LinetestWithCapsule(CoreContext& coreContext,
                                    const Line& line,
                                    const FCollider& capsule,
                                    fp& timeOfIntersection,
                                    FVectorFP& pointOfIntersection) {
        if (!capsule.IsCapsule()) {
            coreContext.logger.LogErrorMessage(
                "CollisionsSimple::linetest",
                "Capsule provided was not a capsule but instead a " + capsule.GetTypeAsString()
            );
            return false;
        }

        return LinetestWithCapsule(coreContext, line, capsule.GetCapsuleMedialLineExtremes(),
                                   capsule.GetCapsuleRadius(), timeOfIntersection, pointOfIntersection);
    }

    /// <summary>
    /// TODO
    /// </summary>
    /// <param name="line">TODO</param>
    /// <param name="capsuleMedianLine">Median line of capsule. Note that this is NOT top and bottom, but rather "A" and "B" in relevant diagrams</param>
    /// <param name="capsuleRadius">TODO</param>
    /// <param name="timeOfIntersection">Time of intersection with respect to line. Between fp{0} and 1 when there is an intersection</param>
    /// <param name="pointOfIntersection">Point of intersection along line, if there is an intersection</param>
    /// <returns>True if intersection occurs, false otherwise</returns>
    bool SimpleCollisions::LinetestWithCapsule(CoreContext& coreContext,
                                               const Line& line,
                                               const Line& capsuleMedianLine,
                                               const fp& capsuleRadius,
                                               fp& timeOfIntersection,
                                               FVectorFP& pointOfIntersection) {
        // Reuse capsule vs capsule logic (or at least the theory behind it)
        // Comparing line vs capsule is just like comparing two capsules with one of fp{0} radius
        // IN SHORT, if line gets within capsule's radius of the capsule's center line, then we know there's an intersection

        // Find distance (squared) between the test line and the median line of the capsule
        fp timeOfIntersectionForCapsuleLine;
        FVectorFP closestPointForCapsuleLine;
        fp distSquared = CollisionHelpers::getClosestPtsBetweenTwoSegments(
            line, capsuleMedianLine,
            timeOfIntersection, timeOfIntersectionForCapsuleLine,
            pointOfIntersection, closestPointForCapsuleLine
        );

        // If (squared) distance smaller than (squared) radius, the line and capsule collide
        bool isColliding = distSquared <= capsuleRadius * capsuleRadius;
        if (!isColliding) {
            return false;
        }

        /// Need to correct time of intersection and point of intersection as this was a test with only lines thus far
        // Case 1: Linetest segment overlaps with capsule medial line
        if (FPMath::isNear(distSquared, fp{0}, fp{0.01f})) {
            // Easy case! Capsule point is really the center of a sphere (due to definition of a capsule + medial line)
            // Thus the real intersection point is capsuleRadius away from the given point
            // TODO: Further optimize this, as feels like a waste with sqrt and division
            fp lineLength = line.getLength();
            fp adjustmentLengthChange = capsuleRadius / lineLength;
            // Eg, if capsule radius is 1 and length is 10, then we need to move 10% along length of line
            timeOfIntersection = timeOfIntersection - adjustmentLengthChange;

            // Edge case: If radius is massive then test line may actually be inside the capsule itself
            if (timeOfIntersection < fp{0}) {
                // For this case, default to intersection happening at beginning of test line as it's within the capsule itself
                timeOfIntersection = fp{0};
                pointOfIntersection = line.start;
            } else {
                // Can either recalculate intersection via going along line OR via adjusting from current point of intersection
                // Either way should work equally well, so decided to work backwards from given point of intersection
                FVectorFP linetestReverseDir = FVectorFP::DirectionNotNormalized(line.end, line.start) / lineLength;
                pointOfIntersection = pointOfIntersection + linetestReverseDir * capsuleRadius;
            }
        } else {
            // Approach: Given we confirmed a collision but test line is NOT intersecting capsule median line,
            //              then it must be intersecting the capsule at some "sphere" point.
            //          ie, the test line is intersecting with some sphere centered on the median line, and we
            //              already know that point as it's closestPointForCapsuleLine!
            //          Thus we can simply do a linetest against the relevant sphere to get exact hit data

            // Create capsule-based sphere that we're confident the line is actually hitting
            FCollider capsuleSphereAtClosestPoint;
            capsuleSphereAtClosestPoint.SetSphere(closestPointForCapsuleLine, capsuleRadius);

            // Do actual raycast check. Note that not checking bool result as this SHOULD intersect no matter what
            fp lineLength = line.getLength();
            FVectorFP testlineDir = FVectorFP::DirectionNotNormalized(line.start, line.end) / lineLength;
            Ray testRay(line.start, testlineDir);
            RaycastWithSphere(
                coreContext, testRay, capsuleSphereAtClosestPoint, timeOfIntersection, pointOfIntersection
            );

            // Currently "time" represents distance along line that collision occurred. Need to convert distance to
            //      linetest definition of time
            timeOfIntersection = timeOfIntersection / lineLength;
            // Edge case: If intersection time is outside normal percentage range (0 <= t <= 1) then line is within
            //              capsule + doesn't intersect with medial line + doesn't intersect outer edges of capsule
            if (timeOfIntersection < fp{0} || timeOfIntersection > fp{1}) {
                // For this case, default to intersection happening at beginning of test line as it's within the capsule itself
                timeOfIntersection = fp{0};
                pointOfIntersection = line.start;
            }
        }

        return true;
    }

    bool SimpleCollisions::isIntersectingAlongAxisAndUpdatePenDepthVars(
        const std::vector<FVectorFP>& boxAVertices, const std::vector<FVectorFP>& boxBVertices,
        const FVectorFP& testAxis,
        fp& smallestPenDepth, FVectorFP& penDepthAxis) {
        // Edge case when two normals are parallel. For now, just continue with other SAT tests
        // Possibly error prone, but the Real-Time Collision Detection book algorithm will handle this differently
        if (testAxis == FVectorFP::Zero()) {
            return true;
        }

        fp currentIntersectionDist = CollisionHelpers::getIntersectionDistAlongAxis(
            boxAVertices, boxBVertices, testAxis);
        if (currentIntersectionDist <= fp{0}) {
            // If not intersecting, then return false immediately as SAT tests are over
            return false;
        }

        // Otherwise, update penetration depth variables and then return true
        if (smallestPenDepth == fp{-1} || currentIntersectionDist < smallestPenDepth) {
            smallestPenDepth = currentIntersectionDist;
            penDepthAxis = testAxis;
        }
        return true;
    }

    // Support function that returns the AABB vertex with index
    FVectorFP SimpleCollisions::getCorner(const FVectorFP& minBoxExtents, const FVectorFP& maxBoxExtents, uint32_t n) {
        // Based on Real-Time Collision Detection book, section 5.5.7 (method called "Corner")
        FVectorFP result;

        result.x = n & 1 ? maxBoxExtents.x : minBoxExtents.x;
        result.y = n & 2 ? maxBoxExtents.y : minBoxExtents.y;
        result.z = n & 4 ? maxBoxExtents.z : minBoxExtents.z;

        return result;
    }

    /// <summary>
    /// Checks if and when a ray intersects an AABB (which is effectively local space check against OBB).
    /// Note: Based on Real-Time Collision Detection, Section 5.3.3
    /// </summary>
    /// <param name="relativeRay">
    /// Assumes in OBB box local space already (and thus using same AABB algorithm).
    /// Origin should be relative to OBB's center (ie, OBB's center is treated as center of world and then rotated).
    /// Direction should also be converted to OBB local space.
    /// </param>
    /// <param name="box">Box to test with.</param>
    /// <param name="timeOfIntersection">
    /// If intersection occurs, this will be time which ray first intersects with the box.
    /// Note that if ray origin is within the box, then this will signify when the ray hits the OBB on the way out.
    /// </param>
    /// <param name="pointOfIntersection">Location where intersection occurs</param>
    /// <returns>True if intersection occurs, false otherwise.</returns>
    bool SimpleCollisions::raycastForAABB(CoreContext& coreContext,
                               const Ray& relativeRay,
                               const FCollider& box,
                               fp& timeOfIntersection,
                               FVectorFP& pointOfIntersection) {
        if (!box.IsBox()) {
            coreContext.logger.LogErrorMessage(
                "CollisionsSimple::raycastForAABB",
                "Provided collider was not a box but instead a " + box.GetTypeAsString()
            );
            return false;
        }

        fp timeOfEarliestHitSoFar = FPMath::minLimit(); // Set to negative max to get first hit/intersection
        fp timeOfLatestHitSoFar = FPMath::maxLimit();
        // Set to max distance ray can travel (for segment), so get last hit on line

        FVectorFP boxMin = -box.GetBoxHalfSize();
        FVectorFP boxMax = box.GetBoxHalfSize();

        // Do checks for all three slabs (axis extents)...
        for (int i = 0; i < 3; i++) {
            // If ray is not moving on this axis...
            if (FPMath::isNear(relativeRay.direction[i], fp{0}, fp{0.0001f})) {
                // No hit if origin not within slab (axis extents) already
                if (relativeRay.origin[i] < boxMin[i] || relativeRay.origin[i] > boxMax[i]) {
                    return false;
                }
            }
            // Otherwise - since ray is moving on this axis - check that ray does enter the slab (axis extents)...
            else {
                // Optimization: Denominator portion computed only once. Also note never dividing by 0 due to earlier if statement check
                fp directionFraction = fp{1} / relativeRay.direction[i];

                // Compute intersection time value of ray with either (near and far relative to ray) plane of axis slab
                // NOTE: Ray is defined by following equation: x = d*t + b, where d = direction on axis and b = starting value
                //       Thus, to solve for t given a specific coordinate value, we'd do t = (x - b)/d
                fp timeOfNearPlaneIntersection = (boxMin[i] - relativeRay.origin[i]) * directionFraction;
                fp timeOfFarPlaneIntersection = (boxMax[i] - relativeRay.origin[i]) * directionFraction;

                // Make sure variable name-expectations are correct for latter check
                if (timeOfNearPlaneIntersection > timeOfFarPlaneIntersection) {
                    FPMath::swap(timeOfNearPlaneIntersection, timeOfFarPlaneIntersection);
                }

                // Compute the intersection of slab intersection intervals
                // (ie, update our earliest and latest hits across all axes thus far)
                timeOfEarliestHitSoFar = FPMath::max(timeOfEarliestHitSoFar, timeOfNearPlaneIntersection);
                timeOfLatestHitSoFar = FPMath::min(timeOfLatestHitSoFar, timeOfFarPlaneIntersection);

                // Exit with no collision as soon as slab intersection becomes empty
                // (This covers both case where ray does not enter slab at all and case where ray doesn't enter all
                //      slabs/axis extents at the same time)
                if (timeOfEarliestHitSoFar > timeOfLatestHitSoFar) return false;
            }
        }

        // If timeOfLatestHitSoFar is less than zero, the ray is intersecting box in negative direction
        //  ie, entire AABB is behind origin of ray and thus no intersection
        // NOTE: Added equality check (<= vs <) as don't want to consider ray that only starts on surface of box to be an intersection
        //          (and checking an "epsilon'/close to 0 value instead of 0 to catch possible minor math inaccuracies)
        if (timeOfLatestHitSoFar <= fp{0.001f}) return false;

        // TODO: Cheaper way to check for starting point in box that's accurate?
        //       Tried doing timeOfEarliestHitSoFar < 0 but false positive when point is on face
        //       See ComplexBoxCapsuleCollisions unit test whenCapsuleJustBarelyTouchingFrontOfLargeBox_statesIsCollidingWithProperPenInfo
        bool doesRayStartInBox = box.IsLocalSpacePtWithinBoxExcludingOnSurface(relativeRay.origin);

        // Check for raycast only touching box surface
        if (!doesRayStartInBox) {
            // If ray inside box then has to exit box so no need to check further in that case
            // Approach:
            // Already covered cases where only touching surface of box once and - by process of elimination - we know
            //      that the ray touches the box more than once on its surface (eg, from one side to other of box).
            //      Thus need to check if intersection "segment" actually goes into the box.
            // Easy way to do so is to get the beginning and end of the "segment", and then check if both points are
            //      on different faces.
            // (Tried to do an approach with line direction and perpendicular vector with another raycast intersection
            //      test, but that seemed to be too expensive)

            // Calculate initial and final intersection locations. Note that position = direction * time + origin (eg, x = a*t + b)
            FVectorFP initialPointOfIntersection = relativeRay.direction * timeOfEarliestHitSoFar + relativeRay.origin;
            FVectorFP finalPointOfIntersection = relativeRay.direction * timeOfLatestHitSoFar + relativeRay.origin;

            // Get faces that each point touches (if any)
            std::vector<FVectorFP> initialPointFaces;
            std::vector<FVectorFP> finalPointFaces;
            box.GetFacesThatLocalSpacePointTouches(initialPointOfIntersection, initialPointFaces);
            box.GetFacesThatLocalSpacePointTouches(finalPointOfIntersection, finalPointFaces);

            // If intersection segment only touches surface and does not go into box, then there should be at least one
            //  face that both beginning and end of segment touch. (Easy to see after drawing rays across a box on paper for a while)
            // SIDE NOTE: Could perhaps do some sort of optimized intersection method (with or without HashSet), but
            //              this is at work 3x3 = 9 loop iterations. Don't find this worth further optimizing atm
            for (const auto& initialPointFace : initialPointFaces) {
                for (const auto& finalPointFace : finalPointFaces) {
                    if (initialPointFace == finalPointFace) {
                        return false; // Confirmed segment only touches surface of box!
                    }
                }
            }
        }

        // As confirmed ray intersects all 3 slabs (axis extents) and not only on surface, we can confirm that there
        //  really was an intersection
        timeOfIntersection = doesRayStartInBox ? timeOfLatestHitSoFar : timeOfEarliestHitSoFar;
        pointOfIntersection = relativeRay.origin + relativeRay.direction * timeOfIntersection;
        return true;
    }
}
