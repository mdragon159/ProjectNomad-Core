#pragma once

#include "RenderEventType.h"
#include "Math/FPQuat.h"
#include "Math/FPVector.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    /**
    * Identifies an event-driven (rather than anim-driven) fx.
    * Contains enough data for renderer to create the expected fx.
    **/
    struct RenderEvent {
        FrameType startingFrame = 0;
        RenderEventType renderEventType = RenderEventType::INVALID_ID;
        FPVector position = FPVector::zero();
        FPQuat rotation = FPQuat::identity();
    };
}
