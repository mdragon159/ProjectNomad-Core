#pragma once

#include <vector>

#include "Line.h"
#include "Utilities/Assertion.h"
#include "Math/FixedPoint.h"
#include "Math/FPMath.h"
#include "Math/FPVector.h"

namespace ProjectNomad {
    /// <summary>TODO. I don't remember why I created this separate class</summary>
    class CollisionHelpers {
    public:
        static fp getEpsilon() {
            return fp{0.00001f};
        }

        /// <summary>
        /// Tests if two boxes are intersecting along a given axis
        /// Heavily inspired by https://gamedev.stackexchange.com/a/92055/67844
        /// </summary>
        /// <param name="boxAVertices">Vector of 8 points representing the vertices of a box under test</param>
        /// <param name="boxBVertices">Vector of 8 points representing the vertices of a box under test</param>
        /// <param name="axis">
        /// Represents the axis to use for a projection separation test. 
        /// Eg, value of <1, 0, 0> tests for intersection along x axis
        /// </param>
        /// <returns>Intersection amount if positive, 0 if objects are touching, and less than 0 represents distance between objects along this axis</returns>
        static fp getIntersectionDistAlongAxis(std::vector<FPVector> boxAVertices, std::vector<FPVector> boxBVertices,
                                               FPVector axis) {
            assertm(boxAVertices.size() == 8, "boxAVertices should contain exactly 8 points");
            assertm(boxBVertices.size() == 8, "boxBVertices should contain exactly 8 points");
            assertm(axis != FPVector::zero(), "Test axis should not be invalid (zero)");

            // Variables to store min and max points along axis for each box
            // TODO: For cleaner code, use fp::Min and fp::Max or such
            fp aMin = fp{-1};
            fp aMax = fp{-1};
            fp bMin = fp{-1};
            fp bMax = fp{-1};

            // Find the respective min and max points along the axis for each box, vertex by vertex
            for (uint32_t i = 0; i < boxAVertices.size(); i++) {
                fp aProjectedDist = axis.dot(boxAVertices.at(i));
                if (aMin == fp{-1} || aProjectedDist < aMin) {
                    aMin = aProjectedDist;
                }
                if (aMax == fp{-1} || aProjectedDist > aMax) {
                    aMax = aProjectedDist;
                }

                fp bProjectedDist = axis.dot(boxBVertices.at(i));
                if (bMin == fp{-1} || bProjectedDist < bMin) {
                    bMin = bProjectedDist;
                }
                if (bMax == fp{-1} || bProjectedDist > bMax) {
                    bMax = bProjectedDist;
                }
            }

            // Finally, calculate the one-dimensional intersection test between our a and b projected segments
            //  Note: If math is unclear, draw out boxes along a 1D axis and do the math by hand. It helps!
            //  Eg, scenario 1 is box a takes up -1 to 0 and box b takes up 0 to 1. longSpan = 2, sumSpan = 2, intersectionDist = 0 (just touching)
            //  Scenario 2 is box a takes up -1 to 0.5, box b is -0.5 to 1. Longspan = 2, sumSpan = 3, intersectionDist = 1
            fp longSpan = FPMath::max(aMax, bMax) - FPMath::min(aMin, bMin);
            // Total length both boxes take up along axis
            fp sumSpan = aMax - aMin + bMax - bMin; // Summed space taken up by both boxes individually
            return sumSpan - longSpan;
        }

        /// <summary>
        /// Returns the squared distance between point c and segment ab
        /// Note: Based on Real-Time Collision, Section 5.1.2.1
        /// </summary>
        /// <param name="segment">Line to compare with</param>
        /// <param name="point">Point to compare with</param>
        /// <returns>Squared distance between provided point and line segment</returns>
        static fp getSquaredDistBetweenPtAndSegment(const Line& segment, const FPVector& point) {
            FPVector segmentVector = segment.end - segment.start;
            FPVector segmentStartToPoint = point - segment.start;
            FPVector segmentEndToPoint = point - segment.end;
            fp e = segmentStartToPoint.dot(segmentVector); // TODO: Var name, what is this visually/real world speaking?

            // Handle cases where c projects outside ab
            if (e <= fp{0}) return segmentStartToPoint.dot(segmentStartToPoint);
            fp f = segmentVector.dot(segmentVector); // TODO: Var name?  Isn't this just sqrd length of segment?
            if (e >= f) return segmentEndToPoint.dot(segmentEndToPoint);

            // Handle cases where c projects onto ab
            return segmentStartToPoint.dot(segmentStartToPoint) - e * e / f;
        }

        /// <summary>
        /// Computes closest points C1 and C2 of S1(s)=P1+s*(Q1-P1) and
        /// S2(t)=P2+t*(Q2-P2), returning s and t. Function result is squared
        /// distance between S1(s) and S2(t)
        /// Note: Directly based on Real-Time Collision Detection, Section 5.1.9
        /// </summary>
        static fp getClosestPtsBetweenTwoSegments(const Line& firstSegment, const Line& secondSegment, 
                                                  fp& timeOfClosestPointForFirstSegment, fp& timeOfClosestPointForSecondSegment,
                                                  FPVector& closestPointOnFirstSegment, FPVector& closestPointOnSecondSegment) {

            return getClosestPtsBetweenTwoSegments(
                firstSegment.start, firstSegment.end, secondSegment.start, secondSegment.end,
                timeOfClosestPointForFirstSegment, timeOfClosestPointForSecondSegment,
                closestPointOnFirstSegment, closestPointOnSecondSegment
            );
        }

        /// <summary>
        /// Computes closest points C1 and C2 of S1(s)=P1+s*(Q1-P1) and
        /// S2(t)=P2+t*(Q2-P2), returning s and t. Function result is squared
        /// distance between S1(s) and S2(t)
        /// Note: Directly based on Real-Time Collision Detection, Section 5.1.9
        /// </summary>
        /// <param name="p1">Point for start of first segment</param>
        /// <param name="q1">Point for end of first segment</param>
        /// <param name="p2">Point for start of second segment</param>
        /// <param name="q2">Point of end of second segment</param>
        /// <param name="s">Resulting "time" along first segment for where the closest point occurs</param>
        /// <param name="t">Resulting "time" along second segment for where the closest point occurs</param>
        /// <param name="c1">Closest point on first segment</param>
        /// <param name="c2">Closest point on second segment</param>
        /// <returns>Squared distance between S1(s) and S2(t)</returns>
        // TODO: Delete in favor of segment-inputs version
        static fp getClosestPtsBetweenTwoSegments(FPVector p1, FPVector q1, FPVector p2, FPVector q2,
                                                  fp& s, fp& t, FPVector& c1, FPVector& c2) {
            // TODO: Rewrite all vars and stuff based on better understanding. Also just adjust it all to personal style
            // TODO: Will epsilon be necessary with fixed point?
            // TODO: All da documentation above

            FPVector d1 = q1 - p1; // Direction vector of segment S1
            FPVector d2 = q2 - p2; // Direction vector of segment S2
            FPVector r = p1 - p2;
            fp a = d1.dot(d1); // Squared length of segment S1, always nonnegative
            fp e = d2.dot(d2); // Squared length of segment S2, always nonnegative
            fp f = d2.dot(r);

            // Check if either or both segments degenerate into points
            if (a <= getEpsilon() && e <= getEpsilon()) { // Both segments degenerate into points
                s = t = fp{0};
                c1 = p1;
                c2 = p2;
                return (c1 - c2).dot(c1 - c2);
            }
            if (a <= getEpsilon()) { // First segment degenerates into a point
                s = fp{0};
                t = f / e; // s = 0 => t = (b*s+f)/e = f/e
                t = FPMath::clamp(t, fp{0.0f}, fp{1.0f});
            }
            else {
                fp c = d1.dot(r);
                if (e <= getEpsilon()) { // Second segment degenerates into a point
                    t = fp{0};
                    s = FPMath::clamp(-c / a, fp{0.0f}, fp{1.0f}); // t = 0 => s =(b*t-c)/a = -c/a
                }
                else { // The general nondegenerate case starts here
                    fp b = d1.dot(d2);
                    fp denom = a * e - b * b; // Always nonnegative

                    // If segments not parallel, compute closest point on L1 to L2 and
                    // clamp to segment S1. Else pick arbitrary s (here 0)
                    if (denom != fp{0.0f}) {
                        s = FPMath::clamp((b * f - c * e) / denom, fp{0.0f}, fp{1.0f});
                    }
                    else s = fp{0};
                    
                    // Compute point on L2 closest to S1(s) using
                    // t = Dot((P1 + D1*s) - P2,D2) / Dot(D2,D2) = (b*s + f) / e
                    t = (b * s + f) / e;

                    // If t in [0,1] done. Else clamp t, recompute s for the new value
                    // of t using s = Dot((P2 + D2*t) - P1,D1) / Dot(D1,D1)= (t*b - c) / a
                    // and clamp s to [0, 1]
                    if (t < fp{0.0f}) {
                        t = fp{0};
                        s = FPMath::clamp(-c / a, fp{0.0f}, fp{1.0f});
                    }
                    else if (t > fp{1.0f}) {
                        t = fp{1};
                        s = FPMath::clamp((b - c) / a, fp{0.0f}, fp{1.0f});
                    }
                }
            }

            c1 = p1 + d1 * s;
            c2 = p2 + d2 * t;
            return (c1 - c2).dot(c1 - c2);
        }

        static void getClosestPtBetweenPtAndSegment(const Line& segment, const FPVector& point, fp& timeOfIntersection,
                                                    FPVector& closestPoint) {
            getClosestPtBetweenPtAndSegment(segment.start, segment.end, point, timeOfIntersection, closestPoint);
        }

        /// <summary>
        /// Given segment ab and point c, computes closest point d on ab.
        /// Also returns t for the position of d, d(t)=a+t*(b - a)
        /// NOTE: Directly based on Real-Time Collision Detection, Section 5.1.2
        /// NOTE x2: There is also a longer version that avoids division, but don't think useful just yet
        /// </summary>
        /// <param name="point">Point to compare with</param>
        /// <param name="segmentStart">Represents start point of line segment</param>
        /// <param name="segmentEnd">Represents end point of line segment</param>
        /// <param name="timeOfIntersection">Time of intersection where time is defined as follows: d(t)=a+t*(b - a)</param>
        /// <param name="closestPoint">Computed closest point between provided point and line segment</param>
        static void getClosestPtBetweenPtAndSegment(const FPVector& segmentStart, const FPVector& segmentEnd, const FPVector& point,
                                                    fp& timeOfIntersection, FPVector& closestPoint) {
        
            FPVector segmentDir = segmentEnd - segmentStart;
        
            // Project c onto ab, computing parameterized position d(t)=a+t*(b � a)
            timeOfIntersection = segmentDir.dot(point - segmentStart) / segmentDir.dot(segmentDir);
        
            // If outside segment, clamp t (and therefore d) to the closest endpoint
            if (timeOfIntersection < fp{0}) timeOfIntersection = fp{0};
            if (timeOfIntersection > fp{1.0f}) timeOfIntersection = fp{1};
        
            // Compute projected position from the clamped t
            closestPoint = segmentStart + timeOfIntersection * segmentDir;
        }
    };
}
