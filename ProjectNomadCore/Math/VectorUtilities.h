#pragma once

#include "FixedPoint.h"
#include "FPVector.h"
#include "FPMath.h"

namespace ProjectNomad {
    class VectorUtilities {
    public:
        static FPVector getAnyPerpendicularVector(const FPVector& normalizedInput) {
            // Choose any arbitrary direction to cross with for a perpendicular vector EXCEPT a parallel vector
            if (normalizedInput != FPVector::up() && normalizedInput != FPVector::down()) {
                return normalizedInput.cross(FPVector::up());
            }

            // Since input is either up or down direction, choose any non-parallel direction to cross with
            return normalizedInput.cross(FPVector::right());
        }
        
        /// <summary>
        /// Get the projection (length * direction) of a given vector in a given direction
        /// </summary>
        /// <param name="testVector">Vector to project onto given direction</param>
        /// <param name="unitVectorToProjectOnto">Direction to project onto. NOTe: Hard assumption that this is a unit vector</param>
        /// <param name="parallelComponent">Resulting parallel component of projection</param>
        static void getParallelVectorProjection(const FPVector& testVector, const FPVector& unitVectorToProjectOnto,
                                                FPVector& parallelComponent) {
            bool isParallelOppositeDir;
            getParallelVectorProjection(testVector, unitVectorToProjectOnto, parallelComponent, isParallelOppositeDir);
        }

        /// <summary>
        /// Get the projection (length * direction) of a given vector in a given direction
        /// </summary>
        /// <param name="testVector">Vector to project onto given direction</param>
        /// <param name="unitVectorToProjectOnto">Direction to project onto. NOTe: Hard assumption that this is a unit vector</param>
        /// <param name="parallelComponent">Resulting parallel component of projection</param>
        /// <param name="isParallelOppositeDir">True if projection is in direction of unit vector, false if in opposite direction</param>
        static void getParallelVectorProjection(const FPVector& testVector, const FPVector& unitVectorToProjectOnto,
                                                FPVector& parallelComponent, bool& isParallelOppositeDir) {
            // IDEA: Double check if input unit vector is actually a unit vector. Throw error if not

            fp magnitudeInProjectionDir = unitVectorToProjectOnto.dot(testVector);
            isParallelOppositeDir = magnitudeInProjectionDir < fp{0}; // Sweet and simple, yay dot products

            // Use simplified projection "formula" due to unit vector assumption (ie, get length in direction then multiply by direction)
            parallelComponent = unitVectorToProjectOnto.dot(testVector) * unitVectorToProjectOnto;
        }

        static void getVectorsRelativeToDir(const FPVector& testVector, const FPVector& unitVectorToProjectOnto,
                                            FPVector& parallelComponent, FPVector& perpendicularComponent) {
            bool isParallelOppositeDir;
            getVectorsRelativeToDir(testVector, unitVectorToProjectOnto, parallelComponent, perpendicularComponent, isParallelOppositeDir);
        }

        static void getVectorsRelativeToDir(const FPVector& testVector, const FPVector& unitVectorToProjectOnto,
                                            FPVector& parallelComponent, FPVector& perpendicularComponent,
                                            bool& isParallelOppositeDir) {
            getParallelVectorProjection(testVector, unitVectorToProjectOnto, parallelComponent, isParallelOppositeDir);
            perpendicularComponent = testVector - parallelComponent;
        }

        /// <summary>
        /// Get angle between two vectors in degrees
        /// </summary>
        /// <param name="a">First vector (doesn't need to be a unit vector)</param>
        /// <param name="b">Second vector (doesn't need to be a unit vector)</param>
        /// <returns>
        /// Angle between vectors in degrees in range of [0, 180].
        /// Note that this method does not make any distinction between "left" and "right".
        /// ie, Forward x Left = 90 and  Forward x Right = 90 as well. Use isXYCrossDotPositive to distinguish the two.
        /// </returns>
        static fp getAngleBetweenVectorsInDegrees(const FPVector& a, const FPVector& b) {
            // TODO: I don't even remember what formula I used. A reference here would be nice
            fp value = a.normalized().dot(b.normalized());

            // Slight errors may still result in a value very slightly greater than magnitude of 1, which would result in
            // erroneous output (due to somehow having an imaginary component for those values w/ inverse cosine?).
            // Avoid this issue by clamping to valid range.
            value = FPMath::clamp(value, fp{-1}, fp{1});
            
            return FPMath::acosD(value);
        }

        static bool isAngleBetweenVectorsInRange(const FPVector& a, const FPVector& b, fp angleRangeInclusive) {
            fp angleBetweenAttackerDirAndFacingDir = getAngleBetweenVectorsInDegrees(a, b);
            return angleBetweenAttackerDirAndFacingDir <= angleRangeInclusive;
        }

        /// <summary>
        /// As name states, this simply returns whether A x B dotted with up direction is positive.
        /// In practice, this is used with getAngleBetweenVectorsInDegrees to determine "left" vs "right" (on XY plane).
        /// </summary>
        /// <param name="a">First vector (doesn't need to be a unit vector)</param>
        /// <param name="b">Second vector (doesn't need to be a unit vector)</param>
        /// <returns>Positive if b is to "right" of a, and negative if b is to "left" of a. See unit tests for examples</returns>
        static bool isXYCrossDotPositive(FPVector a, FPVector b) {
            // TODO: A source link from years back would be nice
            return a.cross(b).dot(FPVector::up()) >= fp{0};
        }

        /**
        * Checks if given direction is within angleRangeInclusive degrees of horizontal plane.
        * ie, this answers the question "Is this direction close to pointing horizontally?"
        * @param inputDir - Input direction to check against horizontal plane
        * @param angleRangeInclusive - allowed angle difference. Must be >= 0
        * @returns true if given direction is close to horizontal, false otherwise
        **/
        static bool IsDirectionCloseToHorizontal(const FPVector& inputDir, fp angleRangeInclusive) {
            // FUTURE NOTE: There *is* some sort of optimization checking just the magnitude of the z value
            // (as x-y vs z ratio varies in conjunction with angle), BUT current implementation certainly works and
            // don't want to superficially think about all this math and waste time on pre-optimizing
            
            // Necessary early check: Make sure there are any horizontal components.
            // If we don't do this check first, then computing horizontal project direction later on may fail due to no horizontal components 
            if (inputDir.x == fp{0} && inputDir.y == fp{0}) {
                return false;
            }
            // Perhaps more harmful than useful pre-optimization check
            if (inputDir.z == fp{0}) {
                return true;
            }

            // Project input direction to horizontal plane to get corresponding horizontal vector
            const FPVector horizontalProjectionDir = zeroOutZ(inputDir).normalized();
            
            // Now we can simply check if angle is within range for input direction vs horizontal plane
            return isAngleBetweenVectorsInRange(inputDir, horizontalProjectionDir, angleRangeInclusive);
        }

        static FPVector zeroOutZ(const FPVector& vector) {
            return FPVector(vector.x, vector.y, fp{0});
        }
        static FPVector zeroOutXY(const FPVector& vector) {
            return FPVector(fp{0}, fp{0}, vector.z);
        }
    };
}
