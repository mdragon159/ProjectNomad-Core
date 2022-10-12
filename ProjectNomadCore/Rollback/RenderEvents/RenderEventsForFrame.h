#pragma once

#include "Utilities/Containers/FlexArray.h"

namespace ProjectNomad {
    /**
    * Tracks necessary render event data (event-driven fx) for a given frame
    * @tparam RenderEventType - Stored type for render events
    **/
    template <typename RenderEventType>
    struct RenderEventsForFrame {
      private:
        // Storage size is currently arbitrarily chosen. Will need to be reviewed in future, but also too many SFX + VFX
        // shouldn't be generated in any given frame anyways for performance
        static constexpr uint32_t kExpectedMaxNewFxPerFrame = 25; // Expected max # of event-driven sfx + vfx generated in any given frame 
        static constexpr uint32_t kExpectedMaxAliveFxPerFrame = 50; // Expected max # of event-driven sfx + vfx being tracked across frames

      public:
        // Events starting this frame. Intended to notify renderer which new event-driven fx should be shown in current frame.
        FlexArray<RenderEventType, kExpectedMaxNewFxPerFrame> newEvents = {};
        
        // Tracking events which began in some previous frame and expected to still be relevant in this frame.
        // Intended to assist renderer to identify which fx were missed in a prior frame post-rollback.
        FlexArray<RenderEventType, kExpectedMaxNewFxPerFrame> pastContinuingEvents = {};

        /**
        * Clears out all data. Intended to prep for reuse (eg, when going from one end of containing circular buffer to other end)
        **/
        void Clear() {
            newEvents = {};
            pastContinuingEvents = {};
        }

        bool IsEmpty() const {
            return newEvents.IsEmpty() && pastContinuingEvents.IsEmpty();
        }
    };
}
