#pragma once
#include "Model/CollisionData.h"
#include "Math/FixedPoint.h"

struct FCollider;

namespace ProjectNomad {
    class Line;
    struct CoreContext;

    class ComplexCollisions {
      public:
        ComplexCollisions() = delete;

        static ImpactResult IsColliding(CoreContext& coreContext, const FCollider& A, const FCollider& B);
        static ImpactResult IsBoxAndBoxColliding(CoreContext& coreContext, const FCollider& boxA, const FCollider& boxB);
        static ImpactResult IsCapsuleAndCapsuleColliding(CoreContext& coreContext, const FCollider& capA, const FCollider& capB);
        static ImpactResult IsSphereAndSphereColliding(CoreContext& coreContext, const FCollider& sphereA, const FCollider& sphereB);
        static ImpactResult IsBoxAndCapsuleColliding(CoreContext& coreContext, const FCollider& box, const FCollider& capsule);
        static ImpactResult IsBoxAndSphereColliding(CoreContext& coreContext, const FCollider& box, const FCollider& sphere);
        static ImpactResult IsCapsuleAndSphereColliding(CoreContext& coreContext, const FCollider& capsule, const FCollider& sphere);

      private:
        // Collision resolution purpose: Have a point in space within box and want to find which direction to push it,
        //                                  such that it's the smallest direction to push the point out of the box
        // NOTE: outPushToBoxFaceDistance is >= 0 (ie, is a magnitude and should not be negative)
        static void CalculateSmallestPushToOutsideBox(const FCollider& box,
                                                      const FVectorFP& localSpacePoint,
                                                      FVectorFP& outPushToOutsideBoxDirInWorldSpace,
                                                      fp& outPushToBoxFaceDistance,
                                                      bool useWrongDirectionFiltering = false,
                                                      const FVectorFP& directionForFiltering = FVectorFP::Zero());

        // curAxisDir = Up, Right, or Forward vectors as working in box local space
        static void CheckIfFaceAlongAxisIsClosestToPoint(const FVectorFP& boxHalfSize,
                                                         const FVectorFP& curAxisDir,
                                                         const FVectorFP& pointToPush,
                                                         fp& smallestDistSoFar,
                                                         FVectorFP& bestPushDirSoFar,
                                                         bool useWrongDirectionFiltering,
                                                         const FVectorFP& directionForFiltering);

        static bool GetBoxCapsuleIntersection(CoreContext& coreContext,
                                              const FCollider& box,
                                              const FCollider& expandedCheckBox,
                                              const Line& boxSpaceCapsuleMedialSegment,
                                              const fp& capsuleRadius,
                                              const fp& capsuleMedialHalfLineLength,
                                              fp& timeOfIntersection,
                                              FVectorFP& pointOfIntersection);

        static ImpactResult CalculateBoxCapsulePenetrationInfo(CoreContext& coreContext,
                                                               const FCollider& box,
                                                               const FCollider& capsule,
                                                               const FCollider& expandedCheckBox,
                                                               const Line& boxSpaceCapsuleMedialLine,
                                                               const fp& timeOfInitialIntersection,
                                                               const FVectorFP& pointOfInitialIntersection);

        /// <summary>Calculate which direction to push an intersecting line out of a box with minimum distance</summary>
        /// <param name="box">Box to push line out of</param>
        /// <param name="middleIntersectionPoint">
        /// Middle of intersection of line. Note that this isn't necessarily the middle point of an ENTIRE line segment but
        /// the merely middle of the segment of the line which intersects the box. 
        /// </param>
        /// <param name="lineDir">Normalized direction of the line</param>
        /// <param name="bestDirToPushLineOutOfBox">Output variable, direction to push middle point out of box</param>
        /// <param name="penetrationMagnitude">Output variable, how much to push middleIntersectionPoint along output direction</param>
        static void GetBestPushInfoOutOfBoxForMiddlePointOfBoxSpaceLine(CoreContext& coreContext,
                                                                        const FCollider& box,
                                                                        const FVectorFP& middleIntersectionPoint,
                                                                        const FVectorFP& lineDir,
                                                                        FVectorFP& bestDirToPushLineOutOfBox,
                                                                        fp& penetrationMagnitude);
        
    };
}
