#pragma once

#include "Utilities/FrameType.h"

namespace ProjectNomad {
    class SimFrame {
      public:
        SimFrame() {
            // Not "efficient", but for consistency set state to what the reset sets
            ResetState();
        }
        
#pragma region Only called by primary manager
        
        void ResetState() {
            mSimFrameCount = 0;
        }

        void SetCurrentFrameCount(FrameType frameCount) {
            mSimFrameCount = frameCount;
        }
        
#pragma endregion
#pragma region High Level Utility Functions

        void IncrementFrameCount() {
            mSimFrameCount++;
        }
        
        FrameType GetCurrentFrameCount() const {
            return mSimFrameCount;
        }

        FrameType GetFrameCountSince(FrameType startingFrame) const {
            return GetCurrentFrameCount() - startingFrame;
        }

#pragma endregion

      private:
        FrameType mSimFrameCount = 0; // TODO: How many seconds/hours is this? Do we have to switch to 64_t?

    };
}
