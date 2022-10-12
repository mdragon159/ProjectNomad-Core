#pragma once

#include "RenderEventsForFrame.h"
#include "Rollback/Model/RollbackSettings.h"
#include "Utilities/FrameType.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    /**
    * Tracks event-driven fx across frames for normal render event processing as well as for post-rollback fx processing.
    * 
    * This tracks all event-driven fx for current frame, kMaxRollbackFrames frames in future, and kMaxRollbackFrames in past.
    * This assures a given event-driven sound or visual effect will be remembered for all rollback needs.
    *
    * Other notes:
    * - This system was designed for "event-driven" fx (in contrast to anim-driven fx) but only in terms of how underlying
    *     data is stored. May be worth removing those mentions from this class
    * - Easiest to understand how this works by drawing a number line. Center represents present and then can use, say,
    *     -10 to -1 to represent history while +1 to +10 represent future.
    * - Note that this only uses in-place memory for ease of copying around... which isn't strictly necessary as not
    *     part of entt::registry, but ah well this works well atm
    *     
    * @tparam RenderEventType - Stored type for render events
    **/
    template <typename RenderEventType>
    class RenderEventTracker {
      public:
        /**
        * Expected to be called once per frame. Handles shifting internal tracking forward by one frame.
        **/
        void IncrementFrame() {
            // Move tracking one frame forward
            mRenderEventTrackRingBuffer.IncrementHead();
            
            // Clear out data for the element that went from the "past" side of the buffer (-kMaxRollbackFrames)
            // to "future" end of buffer (+kMaxRollbackFrames).
            int newFutureValueOffset = RollbackStaticSettings::kMaxRollbackFrames;
            mRenderEventTrackRingBuffer.Get(newFutureValueOffset).Clear();
        }

        /**
        * Adds a new render event to internal tracking of the current frame.
        * @param renderEvent - new event to add
        * @param lifetime - how many frames this event is expected to "live" (or at least worth recreating if missed pre-rollback)
        **/
        void AddNewFxForCurrentFrame(RenderEventType renderEvent, FrameType lifetime) {
            // Add event to new event tracking
            mRenderEventTrackRingBuffer.Get(0).newEvents.Add(renderEvent);

            // Add event to future "past event" tracking as appropriate.
            // Note that we don't care about future frames beyond kMaxRollbackFrames, as we will never rollback beyond
            // the start frame at that point. ie, it's guaranteed that this event was already handled beyond the rollback window 
            for (FrameType i = 1; i < lifetime && i < RollbackStaticSettings::kMaxRollbackFrames + 1; i++) {
                mRenderEventTrackRingBuffer.Get(static_cast<int>(i)).pastContinuingEvents.Add(renderEvent);
            }
        }

        /**
        * Retrieve render events for the current frame. Currently const as see no reason to *not* be const at the moment.
        * @returns reference to current frame's render events
        **/
        const RenderEventsForFrame<RenderEventType>& GetCurrentFrameEvents() const {
            return mRenderEventTrackRingBuffer.Get(0);
        }
        
        // TODO: Implement getting other frame's render events for post-rollback processing
        
      private:
        // Support current frame + history of MaxRollbackFrames + forward view of MaxRollbackFrames. Note that
        // "forward view" is necessary for the "continuingEvents" value tracking (ie, to have O(1) lookup of recent relevant fx)
        static constexpr FrameType kMaxFramesToTrack = RollbackStaticSettings::kMaxRollbackFrames * 2 + 1;

        // Circular buffer of events. Expected to be initialized to empty values.
        // "Head" of buffer should always point to current frame's data, with "previous" entries representing past while
        // "future" entries remembering relevant events that already occurred.
        //
        // ie, if kMaxRollbackFrames = 10, then...
        // 0 = head or current frame,
        // -1 = one previous frame's data up to -10 prior frames, and
        // +1 = next frame's data up to +10 future frames.
        RingBuffer<RenderEventsForFrame<RenderEventType>, kMaxFramesToTrack> mRenderEventTrackRingBuffer = {};
    };
}
