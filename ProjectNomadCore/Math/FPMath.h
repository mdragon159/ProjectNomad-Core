#pragma once

#include "FixedPoint.h"
#include <fpm/math.hpp>

// Following constants based on UnrealMathUtility constants
// TODO: Replace with the fpm e constant
#define FP_VERY_SMALL_NUMBER (fp{1.e-4f})

namespace ProjectNomad {

    class FPMath {
    public:
        static fp getPI() {
            // return 3.14159265f;
            return fp::pi();
        }

        static fp abs(const fp& value) {
            return value < fp{0} ? value * fp{-1} : value;
        }

        static fp sqrt(const fp& value) {
            auto result = fpm::sqrt(value.ToLibraryType());
            return fp::FromLibraryType(result);
        }

        static fp square(const fp& value) {
            return value * value;
        }

        static constexpr fp clamp(const fp& value, const fp& low, const fp& high) {
            if (value < low) {
                return low;
            }

            if (value > high) {
                return high;
            }

            return value;
        }

        // Returns the remainder of numerator / denominator
        // Inspired by UE's GenericPlatformMath::Fmod
        // FUTURE: Look at usages. May not even be worth implementing full fmod and instead hardcoding usage in clampAxis
        static fp fmod(fp numerator, fp denominator) {
            // *looks at UE's implementation*
            // Yeah no, we got a library for a reason
            auto result = fpm::fmod(numerator.ToLibraryType(), denominator.ToLibraryType());
            return fp::FromLibraryType(result);
        }

        static fp clampAxis(fp angle) {
            // Based on UE's Rotator::ClampAxis (and Rotator is similar to our EulerAngles class)

            // returns Angle in the range (-360,360)
            angle = fmod(angle, fp{360.f});

            if (angle < fp{0.f}) {
                // shift to [0,360) range
                angle += fp{360.f};
            }

            return angle;
        }


        static fp normalizeAxis(fp angle) {
            // Based on UE's Rotator::NormalizeAxis

            // returns Angle in the range [0,360)
            angle = clampAxis(angle);

            if (angle > fp{180.f}) {
                // shift to (-180,180]
                angle -= fp{360.f};
            }

            return angle;
        }

        static fp clampAngle(fp angleDegrees, fp minAngleDegrees, fp maxAngleDegrees) {
            // Based on UE's UnrealMath::ClampAngle

            const fp maxDelta = clampAxis(maxAngleDegrees - minAngleDegrees) * fp{0.5f}; // 0..180
            const fp rangeCenter = clampAxis(minAngleDegrees + maxDelta); // 0..360
            const fp deltaFromCenter = normalizeAxis(angleDegrees - rangeCenter); // -180..180

            // maybe clamp to nearest edge
            if (deltaFromCenter > maxDelta) {
                return normalizeAxis(rangeCenter + maxDelta);
            }
            if (deltaFromCenter < -maxDelta) {
                return normalizeAxis(rangeCenter - maxDelta);
            }

            // already in range, just return it
            return normalizeAxis(angleDegrees);
        }

        static constexpr fp min(const fp& a, const fp& b) {
            return !(b < a) ? a : b;
        }

        static constexpr fp max(const fp& a, const fp& b) {
            return (a < b) ? b : a;
        }

        static fp degreesToRadians(const fp& value) {
            return value / 360 * getPI() * 2;
        }

        static fp radiansToDegrees(const fp& value) {
            return value * 180 / getPI();
        }

        static fp cosR(const fp& value) {
            auto result = fpm::cos(value.ToLibraryType());
            return fp::FromLibraryType(result);
        }

        static fp cosD(const fp& value) {
            // TODO: For additional accuracy, do value clamping from following:
            // https://stackoverflow.com/a/31525208/3735890

            return cosR(degreesToRadians(value));
        }

        static fp sinR(const fp& value) {
            auto result = fpm::sin(value.ToLibraryType());
            return fp::FromLibraryType(result);
        }

        static fp sinD(const fp& value) {
            // TODO: For additional accuracy, do value clamping from following:
            // https://stackoverflow.com/a/31525208/3735890

            return sinR(degreesToRadians(value));
        }

        static fp acosR(const fp& value) {
            auto result = fpm::acos(value.ToLibraryType());
            return fp::FromLibraryType(result);
        }

        static fp acosD(const fp& value) {
            return radiansToDegrees(acosR(value));
        }

        static fp asinR(const fp& value) {
            auto result = fpm::asin(value.ToLibraryType());
            return fp::FromLibraryType(result);
        }

        static fp asinD(const fp& value) {
            return radiansToDegrees(asinR(value));
        }

        // Returns angle in radians
        static fp atanR(const fp& y, const fp& x) {
            auto result = fpm::atan2(y.ToLibraryType(), x.ToLibraryType());
            return fp::FromLibraryType(result);
        }

        static fp atanD(const fp& y, const fp& x) {
            return radiansToDegrees(atanR(y, x));
        }

        static void swap(fp& a, fp& b) {
            fp tmp = a;
            a = b;
            b = tmp;
        }

        static bool isNear(const fp& val, const fp& expectedVal, const fp& positiveErrorRange) {
            return val >= expectedVal - positiveErrorRange && val <= expectedVal + positiveErrorRange; 
        }

        static constexpr fp maxLimit() {
            return fp::from_raw_value(std::numeric_limits<fpBaseType>::max());
        }

        static constexpr fp minLimit() {
            return fp::from_raw_value(std::numeric_limits<fpBaseType>::min());
        }

        static uint32_t safeUnsignedDecrement(uint32_t startingValue, uint32_t decrementAmount) {
            if (startingValue < decrementAmount) { // Underflow (less than 0) prevention
                return 0;
            }
            
            return startingValue - decrementAmount;
        }
    };
}
