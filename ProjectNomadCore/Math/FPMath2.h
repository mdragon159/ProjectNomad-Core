#pragma once

#include "FixedPoint.h"
#include "FPVector.h"
#include "FPQuat.h"
#include "FPEulerAngles.h"
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
