#pragma once
#include "Math/FixedPoint.h"

namespace ProjectNomad {
    class CoreConstants {
    public:
        static constexpr fp GetTimePerFrameInSec() {
            return fp{0.0166f};
        }
    };
}
