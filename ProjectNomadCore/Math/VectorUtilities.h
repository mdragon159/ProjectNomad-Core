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

        static FPVector zeroOutZ(const FPVector& vector) {
            return FPVector(vector.x, vector.y, fp{0});
        }
        static FPVector zeroOutXY(const FPVector& vector) {
            return FPVector(fp{0}, fp{0}, vector.z);
        }
    };
}
