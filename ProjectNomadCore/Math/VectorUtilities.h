#pragma once

#include "FixedPoint.h"
#include "FVectorFP.h"
#include "FPMath.h"

namespace ProjectNomad {
    class VectorUtilities {
    public:
        static FVectorFP getAnyPerpendicularVector(const FVectorFP& normalizedInput) {
            // Choose any arbitrary direction to cross with for a perpendicular vector EXCEPT a parallel vector
            if (normalizedInput != FVectorFP::Up() && normalizedInput != FVectorFP::Down()) {
                return normalizedInput.Cross(FVectorFP::Up());
            }

            // Since input is either up or down direction, choose any non-parallel direction to cross with
            return normalizedInput.Cross(FVectorFP::Right());
        }

        /**
        * Get a perpendicular vector which is "vertical" (pointing upwards or downwards). This is useful for taking
        * say a wall normal and finding a vertical perpendicular direction to that normal.
        *
        * If input is the up or down vector, then input will be returned
        * @param normalizedInput - Direction to get perpendicular direction to
        * @returns perpendicular to input direction that is one of two of the more "vertical" perpendicular options
        **/
        static FVectorFP GetVerticalPerpendicularDirection(const FVectorFP& normalizedInput) {
            // Edge case: If already perfectly vertical, then nothing to do
            // NOTE: This almost certainly won't work for up/down directions with minor errors (eg, <0.000031f, 0, 1>)
            if (normalizedInput == FVectorFP::Up() || normalizedInput == FVectorFP::Down()) {
                return normalizedInput;
            }

            // First pick an arbitrary horizontal axis to cross with. This should result in a non-zero result, unless
            //   the input direction is parallel to this axis. NOTE: Could try to directly check
            //   if input is ::left() or ::right(), but inexact inputs have led to issues. Ex: <0.000031f, -1, 0>) 
            FVectorFP tentativeResult = normalizedInput.Cross(FVectorFP::Right()).Normalized();

            // If result unexpectedly led to a non-vertical dir (eg, due to being parallel to last cross product), then
            //   use a different direction
            if (tentativeResult.z == fp{0}) {
                return normalizedInput.Cross(FVectorFP::Forward()).Normalized();
            }

            // Otherwise our previously computed result is fine and thus return that
            return tentativeResult;
        }

        /**
        * Get the perpendicular vector which is "upwards" (pointing upwards or downwards). This is useful for taking
        * say a wall normal and finding the "upwards" direction along the wall's face.
        *
        * If input is the up or down vector, then up direction will be returned
        * @param normalizedInput - Direction to get perpendicular direction to
        * @returns perpendicular to input direction that is most "upwards"
        **/
        static FVectorFP GetUpwardsPerpendicularDirection(const FVectorFP& normalizedInput) {
            FVectorFP verticalPerpVector = GetVerticalPerpendicularDirection(normalizedInput);

            // If facing downwards, then flip to face upwards. Note that need to flip entire vector, as just flipping
            // z value will result in a non-perpendicular vector
            if (verticalPerpVector.z < fp{0}) {
                return verticalPerpVector.Flipped();
            }

            // Otherwise return original perpendicular vec calculation as it's already upwards
            return verticalPerpVector;
        }
        
        /// <summary>
        /// Get the projection (length * direction) of a given vector in a given direction
        /// </summary>
        /// <param name="testVector">Vector to project onto given direction</param>
        /// <param name="unitVectorToProjectOnto">Direction to project onto. NOTe: Hard assumption that this is a unit vector</param>
        /// <param name="parallelComponent">Resulting parallel component of projection</param>
        static void getParallelVectorProjection(const FVectorFP& testVector, const FVectorFP& unitVectorToProjectOnto,
                                                FVectorFP& parallelComponent) {
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
        static void getParallelVectorProjection(const FVectorFP& testVector, const FVectorFP& unitVectorToProjectOnto,
                                                FVectorFP& parallelComponent, bool& isParallelOppositeDir) {
            // IDEA: Double check if input unit vector is actually a unit vector. Throw error if not

            fp magnitudeInProjectionDir = unitVectorToProjectOnto.Dot(testVector);
            isParallelOppositeDir = magnitudeInProjectionDir < fp{0}; // Sweet and simple, yay dot products

            // Use simplified projection "formula" due to unit vector assumption (ie, get length in direction then multiply by direction)
            parallelComponent = unitVectorToProjectOnto.Dot(testVector) * unitVectorToProjectOnto;
        }

        static void getVectorsRelativeToDir(const FVectorFP& testVector, const FVectorFP& unitVectorToProjectOnto,
                                            FVectorFP& parallelComponent, FVectorFP& perpendicularComponent) {
            bool isParallelOppositeDir;
            getVectorsRelativeToDir(testVector, unitVectorToProjectOnto, parallelComponent, perpendicularComponent, isParallelOppositeDir);
        }

        static void getVectorsRelativeToDir(const FVectorFP& testVector, const FVectorFP& unitVectorToProjectOnto,
                                            FVectorFP& parallelComponent, FVectorFP& perpendicularComponent,
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
        static fp getAngleBetweenVectorsInDegrees(const FVectorFP& a, const FVectorFP& b) {
            // TODO: I don't even remember what formula I used. A reference here would be nice
            fp value = a.Normalized().Dot(b.Normalized());

            // Slight errors may still result in a value very slightly greater than magnitude of 1, which would result in
            // erroneous output (due to somehow having an imaginary component for those values w/ inverse cosine?).
            // Avoid this issue by clamping to valid range.
            value = FPMath::clamp(value, fp{-1}, fp{1});
            
            return FPMath::acosD(value);
        }

        static bool isAngleBetweenVectorsInRange(const FVectorFP& a, const FVectorFP& b, fp angleRangeInclusive) {
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
        static bool isXYCrossDotPositive(FVectorFP a, FVectorFP b) {
            // TODO: A source link from years back would be nice
            return a.Cross(b).Dot(FVectorFP::Up()) >= fp{0};
        }

        /**
        * Checks if given direction is within angleRangeInclusive degrees of horizontal plane.
        * ie, this answers the question "Is this direction close to pointing horizontally?"
        * @param inputDir - Input direction to check against horizontal plane
        * @param angleRangeInclusive - allowed angle difference. Must be >= 0
        * @returns true if given direction is close to horizontal, false otherwise
        **/
        static bool IsDirectionCloseToHorizontal(const FVectorFP& inputDir, fp angleRangeInclusive) {
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
            const FVectorFP horizontalProjectionDir = zeroOutZ(inputDir).Normalized();
            
            // Now we can simply check if angle is within range for input direction vs horizontal plane
            return isAngleBetweenVectorsInRange(inputDir, horizontalProjectionDir, angleRangeInclusive);
        }

        static FVectorFP zeroOutZ(const FVectorFP& vector) {
            return FVectorFP(vector.x, vector.y, fp{0});
        }
        static FVectorFP zeroOutXY(const FVectorFP& vector) {
            return FVectorFP(fp{0}, fp{0}, vector.z);
        }

        static FVectorFP RemoveParallelButOppositeComponent(const FVectorFP& velocity, const FVectorFP& direction) {
            // Check for any amount that's parallel but opposite to the "taut rope" direction
            fp curSpeedInRopeDir = velocity.Dot(direction);
            if (curSpeedInRopeDir >= fp{0}) {
                // No parallel but opposite to direction momentum to remove
                return velocity;
            }

            // Remove that parallel but opposite velocity only, just like a taut rope in real life would prevent in real life
            FVectorFP velToRemove = direction * curSpeedInRopeDir;
            return velocity - velToRemove;
        }
    };
}
