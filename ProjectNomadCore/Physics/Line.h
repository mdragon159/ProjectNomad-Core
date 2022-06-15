#pragma once

#include "Math/FixedPoint.h"
#include "Math/FPVector.h"

namespace ProjectNomad {

    /// <summary>
    /// Defines a directional straight line between two points
    /// </summary>
    class Line {
    public:
        FPVector start;
        FPVector end;

        Line() {}
        Line(const FPVector& start, const FPVector& end) : start(start), end(end) {}

        fp getLength() const {
            return (end - start).getLength();
        }

        fp getLengthSquared() const {
            return (end - start).getLengthSquared();
        }

        FPVector getDirection() const {
            return FPVector::direction(start, end);
        }
    };
}
