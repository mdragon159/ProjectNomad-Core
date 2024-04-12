#pragma once

#include "Math/FixedPoint.h"
#include "Math/FVectorFP.h"

namespace ProjectNomad {

    /// <summary>
    /// Defines a directional straight line between two points
    /// </summary>
    class Line {
    public:
        FVectorFP start;
        FVectorFP end;

        Line() {}
        Line(const FVectorFP& start, const FVectorFP& end) : start(start), end(end) {}

        fp getLength() const {
            return (end - start).GetLength();
        }

        fp getLengthSquared() const {
            return (end - start).GetLengthSquared();
        }

        FVectorFP getDirection() const {
            return FVectorFP::Direction(start, end);
        }
    };
}
