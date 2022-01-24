#pragma once

#include "FixedPoint.h"
#include "FPMath.h"
#include "FPVector.h"

namespace ProjectNomad {
    // Credits: Based on https://github.com/BSVino/MathForGameDevelopers/blob/quaternion-transform/math/quaternion.cpp
    // In addition, if reader isn't comfortable with quaternions, highly recommended to watch his series:
    // https://www.youtube.com/watch?v=dttFiVn0rvc&list=PLW3Zl3wyJwWNWsJIPZrmY19urkYHXOH3N&index=1
    class FPQuat {
    public:
        fp w;
        FPVector v;

        FPQuat() : w(fp{0}), v(fp{0}, fp{0}, fp{0}) {}
        FPQuat(fp w, FPVector vector) : w(w), v(vector) {}

        // Building a quaternion from an axis-angle rotation.
        // http://youtu.be/SCbpxiCN0U0
        static FPQuat fromRadians(const FPVector& n, fp angleInRadians) {
            // TODO: Debug assert if n is not normal

            fp w = FPMath::cosR(angleInRadians / 2);
            FPVector v = n * FPMath::sinR(angleInRadians / 2);

            return FPQuat(w, v);
        }

        static FPQuat fromDegrees(const FPVector& n, fp angleInDegrees) {
            return fromRadians(n, FPMath::degreesToRadians(angleInDegrees));
        }

        static FPQuat identity() {
            return FPQuat(fp{1}, FPVector(fp{0}, fp{0}, fp{0}));
        }

        // http://youtu.be/A6A0rpV9ElA
        // ASSUMING unit quaternion! Everything here pretty much assumes that to be fair...
        FPQuat inverted() const {
            return FPQuat(w, -v);
        }

        // Multiplying two quaternions together combines the rotations.
        // http://youtu.be/CRiR2eY5R_s
        FPQuat operator*(const FPQuat& q) const {
            FPQuat result;

            result.w = w * q.w + v.dot(q.v);
            result.v = v * q.w + q.v * w + v.cross(q.v);

            return result;
        }

        // Rotate a vector with this quaternion.
        // http://youtu.be/Ne3RNhEVSIE
        // The basic equation is qpq* (the * means inverse) but we use a simplified version of that equation.
        FPVector operator*(const FPVector& input) const {
            // Could do it this way:
            /*
            const Quaternion& q = (*this);
            return (q * p * q.Inverted()).v;
            */

            // But let's optimize it a bit instead.
            FPVector vCrossInput = v.cross(input);
            return input + vCrossInput * (2 * w) + v.cross(vCrossInput) * fp{2};
        }

        std::string toString() const {
            auto floatW = static_cast<float>(w);

            return std::to_string(floatW) + ", " + v.toString();
        }
    };

    inline bool operator==(const FPQuat& lhs, const FPQuat& rhs) {
        return lhs.w == rhs.w && lhs.v == rhs.v;
    }

    inline bool operator!=(const FPQuat& lhs, const FPQuat& rhs) {
        return !(lhs == rhs);
    }

    inline std::ostream& operator<<(std::ostream& os, const FPQuat& value) {
        os << "FPQuat<" << static_cast<float>(value.w) << ", " << value.v << ">";
        return os;
    }

    // Very lazy workaround to creating a vector4 similar to glm::vec4 (where have x + y + z + w elements)
    // Not using FPQuat directly so can easily find these use cases in case want to refactor
    using FPVector4 = FPQuat;
}
