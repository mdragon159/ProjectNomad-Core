#pragma once

#include "Math/FixedPoint.h"
#include "Math/FVectorFP.h"

struct FCollider;

namespace ProjectNomad {
    class Line;
    class Ray;
    struct CoreContext;

    class SimpleCollisions {
    public:
        SimpleCollisions() = delete;
        
        static bool IsColliding(CoreContext& coreContext, const FCollider& A, const FCollider& B);
        static bool IsBoxAndBoxColliding(CoreContext& coreContext, const FCollider& boxA, const FCollider& boxB);
        static bool IsCapsuleAndCapsuleColliding(CoreContext& coreContext, const FCollider& capA, const FCollider& capB);
        static bool IsSphereAndSphereColliding(CoreContext& coreContext, const FCollider& sphereA, const FCollider& sphereB);
        static bool IsBoxAndCapsuleColliding(CoreContext& coreContext, const FCollider& box, const FCollider& capsule);
        static bool IsBoxAndSphereColliding(CoreContext& coreContext, const FCollider& box, const FCollider& sphere);
        static bool IsCapsuleAndSphereColliding(CoreContext& coreContext, const FCollider& capsule, const FCollider& sphere);

        // Extras below for ease of typing. Mostly due to ease of scanning for consistency in isColliding
        static bool IsCapsuleAndBoxColliding(CoreContext& coreContext, const FCollider& capsule, const FCollider& box);
        static bool IsSphereAndBoxColliding(CoreContext& coreContext, const FCollider& sphere, const FCollider& box);
        static bool IsSphereAndCapsuleColliding(CoreContext& coreContext, const FCollider& sphere, const FCollider& capsule);

        

        /// <summary>
        /// Check if and when a ray and sphere intersect
        /// Based on Game Physics Cookbook, Ch 7
        /// </summary>
        /// <param name="ray">Ray to test with</param>
        /// <param name="sphere">Sphere to test with</param>
        /// <param name="timeOfIntersection">If intersection occurs, this will be time which ray first intersects with sphere</param>
        /// <param name="pointOfIntersection">Location where intersection occurs</param>
        /// <returns>True if intersection occurs, false otherwise</returns>
        static bool RaycastWithSphere(CoreContext& coreContext,
                                      const Ray& ray,
                                      const FCollider& sphere,
                                      fp& timeOfIntersection,
                                      FVectorFP& pointOfIntersection);
        
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
        static bool RaycastWithBox(CoreContext& coreContext,
                                   const Ray& ray,
                                   const FCollider& box,
                                   fp& timeOfIntersection,
                                   FVectorFP& pointOfIntersection);

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
        static bool LinetestWithBox(CoreContext& coreContext,
                                    const Line& line,
                                    const FCollider& box,
                                    fp& timeOfIntersection,
                                    FVectorFP& pointOfIntersection);

        static bool LinetestWithCapsule(CoreContext& coreContext,
                                        const Line& line,
                                        const FCollider& capsule,
                                        fp& timeOfIntersection,
                                        FVectorFP& pointOfIntersection);

        /// <summary>
        /// TODO
        /// </summary>
        /// <param name="line">TODO</param>
        /// <param name="capsuleMedianLine">Median line of capsule. Note that this is NOT top and bottom, but rather "A" and "B" in relevant diagrams</param>
        /// <param name="capsuleRadius">TODO</param>
        /// <param name="timeOfIntersection">Time of intersection with respect to line. Between fp{0} and 1 when there is an intersection</param>
        /// <param name="pointOfIntersection">Point of intersection along line, if there is an intersection</param>
        /// <returns>True if intersection occurs, false otherwise</returns>
        static bool LinetestWithCapsule(CoreContext& coreContext,
                                        const Line& line,
                                        const Line& capsuleMedianLine,
                                        const fp& capsuleRadius,
                                        fp& timeOfIntersection,
                                        FVectorFP& pointOfIntersection);


        
#pragma region Collision Helpers (ie, should be private but not cuz ComplexCollisions usage)

        static bool isIntersectingAlongAxisAndUpdatePenDepthVars(const std::vector<FVectorFP>& boxAVertices,
                                                                 const std::vector<FVectorFP>& boxBVertices,
                                                                 const FVectorFP& testAxis,
                                                                 fp& smallestPenDepth, FVectorFP& penDepthAxis);

        // Support function that returns the AABB vertex with index
        static FVectorFP getCorner(const FVectorFP& minBoxExtents, const FVectorFP& maxBoxExtents, uint32_t n);

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
        static bool raycastForAABB(CoreContext& coreContext,
                            const Ray& relativeRay,
                            const FCollider& box,
                            fp& timeOfIntersection,
                            FVectorFP& pointOfIntersection);

#pragma endregion 
    };
}
