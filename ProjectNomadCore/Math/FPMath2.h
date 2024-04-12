#pragma once

#include "FixedPoint.h"
#include "FVectorFP.h"
#include "FQuatFP.h"
#include "FPEulerAngles.h"
#include "VectorUtilities.h"

namespace ProjectNomad {

    /// <summary>
    /// Contains utility math functions which may cause problems together due to
    /// header file inter-dependencies (eg, FPVector). There should be a better
    /// solution out there, but this is pretty simple and works if pretty darn ugly
    /// TODO: Rename and refactor more appropriately once more is added
    /// </summary>
    class FPMath2 {
    public:
        // No promises for behavior if zeroToOne is outside range
        static fp lerp(const fp& a, const fp& b, const fp& alpha) {
            return a + (b - a) * alpha;
        }

        static fp bezierInterp(const fp& a, const fp& b, const fp& alpha) {
            return lerp(a, b, bezierBlend(alpha));
        }

        // No promises for behavior if zeroToOne is outside range
        static FVectorFP lerp(const FVectorFP& a, const FVectorFP& b, const fp& alpha) {
            return a + (b - a) * alpha;
        }

        static FVectorFP interpTo(const FVectorFP& current, const FVectorFP& target, fp interpSpeed) {
            // Based on UnrealMath::VInterpTo

            // If no interp speed, jump to target value
            if (interpSpeed <= fp{0.f})
            {
                return target;
            }

            // Distance to reach
            const FVectorFP dist = target - current;

            // If distance is too small, just set the desired location
            if (dist.GetLengthSquared() < FP_VERY_SMALL_NUMBER)
            {
                return target;
            }

            // Small delta movement. Clamp so we don't overshoot
            const FVectorFP deltaMove =
                dist * FPMath::clamp( FrameRate::TimePerFrameInSec() * interpSpeed, fp{0.f}, fp{1.f});

            return current + deltaMove;
        }

        static EulerAngles QuatToEuler(const FQuatFP& quat) {
            // "Proper" way to do so: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Quaternion_to_Euler_angles_conversion
            // However, we already implemented quat to dir vector and dir vector to euler. Thus, just reuse that.
            // NOTE: It'd likely be far more efficient to work in dir vector space than to work in euler space, esp if
            //       converting back to quat
            
            return DirVectorToEuler(QuatToDirVector(quat));
        }

        static FQuatFP eulerToQuat(const EulerAngles& euler) {
            // Solid reference: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Euler_angles_to_quaternion_conversion
            // Another good algorithm reference: https://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/index.htm

            // Abbreviations for the various angular functions
            fp cy = FPMath::cosD(euler.yaw * fp{0.5f});
            fp sy = FPMath::sinD(euler.yaw * fp{0.5f});
            fp cp = FPMath::cosD(euler.pitch * fp{0.5f});
            fp sp = FPMath::sinD(euler.pitch * fp{0.5f});
            fp cr = FPMath::cosD(euler.roll * fp{0.5f});
            fp sr = FPMath::sinD(euler.roll * fp{0.5f});

            FQuatFP result;
            result.w = cr * cp * cy + sr * sp * sy;
            result.v.x = sr * cp * cy - cr * sp * sy;
            result.v.y = cr * sp * cy + sr * cp * sy;
            result.v.z = cr * cp * sy - sr * sp * cy;
            return result;
        }

        static FVectorFP eulerToDirVector(const EulerAngles& euler) {
            // Based on the following: https://stackoverflow.com/a/1568687/3735890
            // Note that this doesn't take into account roll as it doesn't affect result here (following yaw -> pitch -> roll rotation order)

            FVectorFP result;
            result.x = FPMath::cosD(euler.yaw) * FPMath::cosD(euler.pitch);
            result.y = FPMath::sinD(euler.yaw) * FPMath::cosD(euler.pitch);
            result.z = FPMath::sinD(euler.pitch);

            return result;
        }

        // NOTE: Method is untested (and currently unused). Needs unit tests!
        static EulerAngles DirVectorToEuler(const FVectorFP& input) {
            // High level approach: Doing the reverse of eulerToDirVector method
            EulerAngles result;

            // Pitch is very simple, as it only depends on z value
            result.pitch = FPMath::asinD(input.z);

            // If not pointing straight up or down (where yaw is irrelevant), then compute yaw
            //    Really doing this for covering divide by zero edge case and thus explicitly checking that,
            //     even though it may be more computationally expensive than just checking pitch or x + y values
            fp cosOfPitch = FPMath::cosD(result.pitch);
            if (cosOfPitch != fp{0}) { // Divide by zero check
                fp inverseCosineInput = input.x / cosOfPitch; // // Could use x or y (with inverse sin), but arbitrarily chose x
                
                if (FPMath::isNear(inverseCosineInput, fp{1}, fp{0.01f})) { // Edge case where acos operation fails
                    result.yaw = fp{0};
                }
                else if (FPMath::isNear(inverseCosineInput, fp{-1}, fp{0.01f})) { // Edge case where acos operation fails
                    result.yaw = fp{180};
                }
                else {
                    result.yaw = FPMath::acosD(input.x / cosOfPitch);
                }
            }

            // And done!
            // Note that we don't care about roll here, as it doesn't affect the direction the vector is pointing in
            // (given that we're following yaw -> pitch -> roll rotation order)
            return result;
        }

        /**
        * Simple method to show how to convert a quat to direction vector notation
        * @param input - rotation to represent as a direction vector
        * @returns direction vector with provided input rotation
        **/
        static FVectorFP QuatToDirVector(const FQuatFP& input) {
            return input * FVectorFP::Forward();
        }

        /// <returns>
        /// Returns a quaternion which represents rotation necessary to rotate FPVector::Forward in order to match
        /// the provided rotation vector.
        /// </returns>
        static FQuatFP dirVectorToQuat(const FVectorFP& targetVec) {
            return dirVectorToQuat(targetVec, FVectorFP::Forward());
        }

        /// <returns>
        /// Returns a quaternion which represents rotation necessary to rotate referenceVec to match the provided
        /// rotation vector.
        /// </returns>
        static FQuatFP dirVectorToQuat(const FVectorFP& targetVec, const FVectorFP& referenceVec) {
            // Goal here is to create an axis-angle representation that would rotate referenceVec to become rotationVec
            // References: Sorry, don't really have any references here. This is more so based on what a quaternion is
            //              and choosing to calculate the quat like this due to intended usage (eg, Transform rotation)
            
            // Rotation axis: Simply get direction perpendicular to both directions
            // NOTE: Order matters here. Why this order? Too lazy to figure out the math, but guess and checked with unit tests
            FVectorFP rotationAxis = referenceVec.Cross(targetVec);

            // If rotation and reference vectors are parallel...
            if (rotationAxis == FVectorFP::Zero()) {
                // Need any perpendicular vector. The "easy" (least thinking) way to do this is to pick any other vector
                // then do another cross product with one of the input vectors.
                // NOTE:
                //      Since this is practically only used with the overload, we COULD just hardcode another perpendicular vector in.
                //      However, gonna do this the most flexible way for now until this is relevant in an optimization pass
                FVectorFP secondTestVec = FVectorFP::Up();
                rotationAxis = secondTestVec.Cross(referenceVec);

                // If our guessed not-parallel vec is actually parallel, then use a different (and certainly not parallel) vector
                if  (rotationAxis == FVectorFP::Zero()) {
                    secondTestVec = FVectorFP::Forward();
                    rotationAxis = secondTestVec.Cross(referenceVec);
                }
            }

            // The cross product of two unit vectors is *not* always a unit vector. Thus need to re-normalize before using further
            rotationAxis.Normalize();

            // Rotation amount around axis: Get angle between the vectors
            // Using degrees due to personal preference, and I THINK it's more accurate due to less decimal points
            fp rotationAmountInDegrees = VectorUtilities::getAngleBetweenVectorsInDegrees(targetVec, referenceVec);

            // And that's it! We have all the info we need to rotate referenceVec into rotationVec
            // by multiplying by the resulting quat
            return FQuatFP::fromDegrees(rotationAxis, rotationAmountInDegrees);
        }

        /**
        * Returns a quaternion which represents a yaw-only rotation.
        * Assumes input is a horizontal-only dir (ie, no z value and actual normalized direction).
        *
        * Created as a way to workaround certain inputs to dirVectorToQuat() leading to upside down characters...
        * Which is understandable, as there are many ways to rotate to a certain direction (like backwards forward dir).
        * This method instead assures we *only* have "yaw" rotation and no other components.
        * Specifically, this assures that the rotation vector within the quat is FPVector::up().
        * @param desiredHorizontalDir - Desired direction that FPQuat should rotate a FPVector::forward() direction is.
        *                               Should have no vertical (z) component and expected to be normalized.
        * @returns "Yaw"-only quaternion representing the input direction
        **/
        static FQuatFP HorizontalDirVectorToYawOnlyQuat(const FVectorFP& desiredHorizontalDir) {
            // Approach: Basically take dirVectorToQuat() but use constant rotation axis and reference axis
            
            /// Define some constants for clarity. Theoretically could replace these for other use cases in future.
            // Result rotation axis: This is the "yaw-only" part
            static const FVectorFP kRotationAxis = FVectorFP::Up();
            // Reference axis: Want to be explicit that the output is as if applied to this vector (which is the game's standard)
            static const FVectorFP kReferenceAxis = FVectorFP::Forward();

            // Rotation amount around axis: Get angle between the vectors.
            //   Note: Due to both being in horizontal plane (and perpendicular to vertical axis), this represents yaw
            //              rotation in degrees.
            //      Using degrees due to personal preference, and I THINK it's more accurate due to less decimal points.
            fp rotationAmountInDegrees = VectorUtilities::getAngleBetweenVectorsInDegrees(kReferenceAxis, desiredHorizontalDir);
            if (!VectorUtilities::isXYCrossDotPositive(kReferenceAxis, desiredHorizontalDir)) {
                rotationAmountInDegrees = -rotationAmountInDegrees;
            }
            
            // Given expectations are met (ie that input is a horizontal dir), then nothing more to do!
            return FQuatFP::fromDegrees(kRotationAxis, rotationAmountInDegrees);
        }

    private:
        /// <summary>
        /// Returns value zero to one with ease in and out provided by a Bezier curve.
        /// For source + visualization, see Solution 2: https://stackoverflow.com/a/25730573/3735890
        /// </summary>
        /// <param name="alpha">Control value. Should be 0 to 1 (but outside values are still valid)</param>
        /// <returns>Returns a value 0 to 1 based on alpha</returns>
        static fp bezierBlend(const fp& alpha) {
            fp val3 = fp{3};
            fp val2 = fp{2};
            return alpha * alpha * (val3 - val2 * alpha);
        }

        /// <summary>
        /// Returns value zero to one with ease in and out provided by a parametric function.
        /// For source + visualization, see Solution 3: https://stackoverflow.com/a/25730573/3735890
        /// </summary>
        /// <param name="alpha">Control value. Should be 0 to 1 (but outside values are still valid)</param>
        /// <returns>Returns a value 0 to 1 based on alpha</returns>
        static fp parametricBlend(const fp& alpha) {
            fp val1 = fp{1};
            fp val2 = fp{2};
            fp sqAlpha = alpha * alpha;
            return sqAlpha / (val2 * (sqAlpha - alpha) + val1);
        }
    };
}
