#pragma once

#include "FixedPoint.h"
#include "FPVector.h"
#include "FPQuat.h"
#include "FPEulerAngles.h"
#include "VectorUtilities.h"
#include "GameCore/CoreConstants.h"

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
        static FPVector lerp(const FPVector& a, const FPVector& b, const fp& alpha) {
            return a + (b - a) * alpha;
        }

        static FPVector interpTo(const FPVector& current, const FPVector& target, fp interpSpeed) {
            // Based on UnrealMath::VInterpTo

            // If no interp speed, jump to target value
            if (interpSpeed <= fp{0.f})
            {
                return target;
            }

            // Distance to reach
            const FPVector dist = target - current;

            // If distance is too small, just set the desired location
            if (dist.getLengthSquared() < FP_VERY_SMALL_NUMBER)
            {
                return target;
            }

            // Small delta movement. Clamp so we don't overshoot
            fp timePerFrameInSec = CoreConstants::GetTimePerFrameInSec();
            const FPVector deltaMove =
                dist * FPMath::clamp( timePerFrameInSec * interpSpeed, fp{0.f}, fp{1.f});

            return current + deltaMove;
        }

        // TODO: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Quaternion_to_Euler_angles_conversion
        static EulerAngles quatToEuler(const FPQuat& quat) {
            return EulerAngles::zero();
        }

        static FPQuat eulerToQuat(const EulerAngles& euler) {
            // Solid reference: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles#Euler_angles_to_quaternion_conversion
            // Another good algorithm reference: https://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/index.htm

            // Abbreviations for the various angular functions
            fp cy = FPMath::cosD(euler.yaw * fp{0.5f});
            fp sy = FPMath::sinD(euler.yaw * fp{0.5f});
            fp cp = FPMath::cosD(euler.pitch * fp{0.5f});
            fp sp = FPMath::sinD(euler.pitch * fp{0.5f});
            fp cr = FPMath::cosD(euler.roll * fp{0.5f});
            fp sr = FPMath::sinD(euler.roll * fp{0.5f});

            FPQuat result;
            result.w = cr * cp * cy + sr * sp * sy;
            result.v.x = sr * cp * cy - cr * sp * sy;
            result.v.y = cr * sp * cy + sr * cp * sy;
            result.v.z = cr * cp * sy - sr * sp * cy;
            return result;
        }

        static FPVector eulerToDirVector(const EulerAngles& euler) {
            // Based on the following: https://stackoverflow.com/a/1568687/3735890
            // Note that this doesn't take into account roll as it doesn't affect result here (following yaw -> pitch -> roll rotation order)

            FPVector result;
            result.x = FPMath::cosD(euler.yaw) * FPMath::cosD(euler.pitch);
            result.y = FPMath::sinD(euler.yaw) * FPMath::cosD(euler.pitch);
            result.z = FPMath::sinD(euler.pitch);

            return result;
        }

        // NOTE: Method is untested (and currently unused). Needs unit tests!
        static EulerAngles dirVectorToEuler(const FPVector& input) {
            // High level approach: Doing the reverse of eulerToDirVector method
            EulerAngles result;

            // Pitch is very simple, as it only depends on z value
            result.pitch = FPMath::asinD(input.z);

            // If not pointing straight up or down (where yaw is irrelevant), then compute yaw
            //    Really doing this for covering divide by zero edge case and thus explicitly checking that,
            //     even though it may be more computationally expensive than just checking pitch or x + y values
            fp cosOfPitch = FPMath::cosD(result.pitch);
            if (cosOfPitch != fp{0}) {
                // Could use x or y (with inverse sin), but arbitrarily chose x
                result.yaw = FPMath::acosD(input.x / cosOfPitch);
            }

            // And done!
            // Note that we don't care about roll here, as it doesn't affect the direction the vector is pointing in
            // (given that we're following yaw -> pitch -> roll rotation order)
            return result;
        }

        /// <returns>
        /// Returns a quaternion which represents rotation necessary to rotate FPVector::Forward in order to match
        /// the provided rotation vector.
        /// </returns>
        static FPQuat dirVectorToQuat(const FPVector& targetVec) {
            return dirVectorToQuat(targetVec, FPVector::forward());
        }

        /// <returns>
        /// Returns a quaternion which represents rotation necessary to rotate referenceVec to match the provided
        /// rotation vector.
        /// </returns>
        static FPQuat dirVectorToQuat(const FPVector& targetVec, const FPVector& referenceVec) {
            // Goal here is to create an axis-angle representation that would rotate referenceVec to become rotationVec
            // References: Sorry, don't really have any references here. This is more so based on what a quaternion is
            //              and choosing to calculate the quat like this due to intended usage (eg, Transform rotation)
            
            // Rotation axis: Simply get direction perpendicular to both directions
            // NOTE: Order matters here. Why this order? Too lazy to figure out the math, but guess and checked with unit tests
            FPVector rotationAxis = referenceVec.cross(targetVec);

            // If rotation and reference vectors are parallel...
            if (rotationAxis == FPVector::zero()) {
                // Need any perpendicular vector. The "easy" (least thinking) way to do this is to pick any other vector
                // then do another cross product with one of the input vectors.
                // NOTE:
                //      Since this is practically only used with the overload, we COULD just hardcode another perpendicular vector in.
                //      However, gonna do this the most flexible way for now until this is relevant in an optimization pass
                FPVector secondTestVec = FPVector::up();
                rotationAxis = secondTestVec.cross(referenceVec);

                // If our guessed not-parallel vec is actually parallel, then use a different (and certainly not parallel) vector
                if  (rotationAxis == FPVector::zero()) {
                    secondTestVec = FPVector::forward();
                    rotationAxis = secondTestVec.cross(referenceVec);
                }
            }

            // The cross product of two unit vectors is *not* always a unit vector. Thus need to re-normalize before using further
            rotationAxis.normalize();

            // Rotation amount around axis: Get angle between the vectors
            // Using degrees due to personal preference, and I THINK it's more accurate due to less decimal points
            fp rotationAmountInDegrees = VectorUtilities::getAngleBetweenVectorsInDegrees(targetVec, referenceVec);

            // And that's it! We have all the info we need to rotate referenceVec into rotationVec
            // by multiplying by the resulting quat
            return FPQuat::fromDegrees(rotationAxis, rotationAmountInDegrees);
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
