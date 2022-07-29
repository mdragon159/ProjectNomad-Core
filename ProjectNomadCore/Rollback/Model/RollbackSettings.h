#pragma once

#include "Utilities/FrameType.h"

namespace ProjectNomad {
    struct RollbackSettings {
        bool useLockstep = false;

        // If this is negative then "negative input delay" feature will be used.
        // "Negative input delay" best explained by this: https://medium.com/@yosispring/input-buffering-action-canceling-and-also-forbidden-knowledge-47a3f8a95151
        // In short, game will predict local player's inputs for number of negative input frames, which is useful for
        // 
        int localInputDelay = 3;
        FrameType onlineInputDelay = 3; // Using FrameType as not supporting "negative input delay" for online play
    };

    struct RollbackStaticSettings {
        static constexpr FrameType kMaxInputDelay = 10;
        
        // Rollback up to this number of frames. Does not impact local "negative input delay" feature.
        // ie, if for some reason trying to test negative input delay locally that's bigger than this number, than
        // the rollback window will simply be the negative input delay.
        static constexpr FrameType kMaxRollbackFrames = 10;
    }; 
}
