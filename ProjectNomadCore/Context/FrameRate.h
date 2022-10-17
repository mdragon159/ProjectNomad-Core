#pragma once

#include "Math/FixedPoint.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    /**
    * Static utility class to provide values for frame rate. ie, declare everything frame rate related all in one place
    * so that it's easy to refer to as well as update all in one clear place.
    **/
    class FrameRate {
      public:
        static constexpr FrameType kGameplayFrameRate = 60;

        static consteval fp TimePerFrameInMicroSec() {
            return fp{1000} / fp{kGameplayFrameRate};
        }

        // TODO: Figure out the constexpr/consteval issues with fp!
        static fp TimePerFrameInSec() {
            return fp{1} / fp{kGameplayFrameRate};
        }

        static consteval float FloatTimePerFrameInSec() { // For ease of use in engine (Unreal) side. Should NEVER be used in SimLayer!
            return 1.f / kGameplayFrameRate;
        }

        /**
        * Used to declare a frame value in terms of 30fps. Intended to reduce time for updating hardcoded values.
        * Eg, if gameplay frame rate is 60fps (2x 30fps) and 5 is passed in, then 10 will be returned.
        * And if gameplay frame rate is 30fps and 5 is passed in, then 5 will be returned.
        * @param valueAs30Fps - input 30fps frame value
        * @returns input frame in current frame rate
        **/
        static consteval FrameType As30FpsFrame(FrameType valueAs30Fps) {
            // Note that there's a concerning edge case when kGameplayFrameRate < 30.
            // Namely, an input of 1 will round down to a 0. However, there are no intentions of testing an fps < 30
            return valueAs30Fps * kGameplayFrameRate / 30;
        }
    };
}
