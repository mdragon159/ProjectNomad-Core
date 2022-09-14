#pragma once
#include "OldRollbackStaticSettings.h"
#include "Input/PlayerInput.h"
#include "Utilities/FrameType.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    // Buffer to retrieve inputs. Head represents current frame + input delay
    //      Size is MaxRollblackFrames + MaxInputDelay to cover worst edge case for data storage
    //      (ie, if have max input delay AND need to rollback to the furthest back possible frame)
    static constexpr FrameType INPUT_BUFFER_SIZE = OldRollbackStaticSettings::MaxRollbackFrames + OldRollbackStaticSettings::MaxInputDelay;
    using OldRollbackInputBuffer = RingBuffer<PlayerInput, INPUT_BUFFER_SIZE>;
}
