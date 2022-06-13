#pragma once

#include "Math/FPVector.h"

namespace ProjectNomad {

    /// <summary>
    /// Defines a Ray (point in space with a direction that goes on forever)
    /// Largely taken from Game Physics Cookbook, Chapter 7
    /// </summary>
    class Ray {
    public:
        FPVector origin;
        FPVector direction;

        Ray() : direction(fp{0}, fp{0}, fp{1}) {}

        Ray(const FPVector& origin, const FPVector& direction) : origin(origin), direction(direction) {
            normalizeDirection(); // Not most efficient option but safe
        }

        static Ray fromPoints(const FPVector& from, const FPVector& to) {
            return Ray(from, (to - from).normalized());
        }

    private:
        void normalizeDirection() {
            direction.normalize();
        }
    };
}
