#pragma once
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    class RollbackStaticSettings {
    public:
        static constexpr bool UseLockstep = false;
        
        static constexpr FrameType OfflineInputDelay = 3;
        static constexpr FrameType OnlineInputDelay = 3;
        static constexpr FrameType MaxInputDelay = 10;

        static constexpr FrameType MaxRollbackFrames = 10; // Rollback up to this number of frames
    };
}
