#pragma once

#include "Math/FixedPoint.h"
#include "Math/FVectorFP.h"

namespace ProjectNomad {

    /// <summary>
    /// Defines a Ray (point in space with a direction that goes on forever)
    /// Largely taken from Game Physics Cookbook, Chapter 7
    /// </summary>
    class Ray {
    public:
        FVectorFP origin;
        FVectorFP direction;

        Ray() : direction(fp{0}, fp{0}, fp{1}) {}

        Ray(const FVectorFP& origin, const FVectorFP& direction) : origin(origin), direction(direction) {
            normalizeDirection(); // Not most efficient option but safe
        }

        static Ray fromPoints(const FVectorFP& from, const FVectorFP& to) {
            return Ray(from, (to - from).Normalized());
        }

    private:
        void normalizeDirection() {
            direction.Normalize();
        }
    };
}
