#pragma once

#include "Context/FrameRate.h"
#include "Utilities/SharedUtilities.h"

namespace ProjectNomad {
    /// <summary>Encapsulates responsibility for appropriate timing of gameplay.</summary>
    class RollbackTimeManager {
      public:
        RollbackTimeManager() = default;
        // Special constructor for unit tests so can easily, quickly, and precisely test behavior
        explicit RollbackTimeManager(std::function<uint64_t()> timeRetriever) : mTimeRetriever(std::move(timeRetriever)) {}
        
        void Start() {
            // Reset state back to defaults
            mLastUpdateTimeInMicroSec = 0;
            mIsPaused = false;
            mShouldNextUpdateHandleUnpausing = false;
            mPauseTimeInMs = 0;
        }

        bool IsPaused() const {
            return mIsPaused;
        }
        void Pause() {
            mIsPaused = true;
            mPauseTimeInMs = mTimeRetriever();
        }
        void Resume() {
            mIsPaused = false;
            mShouldNextUpdateHandleUnpausing = true;
        }

        /**
        * Calculates how many gameplay frames need to be handled in order to maintain desired fps simulation
        * @returns number of gameplay frames that need to be processed to maintain desired fps simulation
        **/
        FrameType CheckHowManyFramesToProcess() {
            if (mIsPaused) {
                return 0;
            }
            
            FrameType numberOfFramesToProcess = 0;

            // Is this the first update call?
            if (mLastUpdateTimeInMicroSec == 0) {
                // Do necessary work for first frame
                numberOfFramesToProcess = 1;

                // Prepare for next run
                mLastUpdateTimeInMicroSec = mTimeRetriever();
            }
            // Otherwise process however many frames we need to catch up
            else {
                uint64_t currentTimeInMicroSec = mTimeRetriever();

                // Special unpausing case: Make sure to only handle 1 frame max
                if (mShouldNextUpdateHandleUnpausing) {
                    // If just unpaused the game...
                    mShouldNextUpdateHandleUnpausing = false;

                    // If at least 1 frame's worth of time passed, then only process a single frame when unpausing
                    // (ie, prevent being able to "rapidly" skip forward by rapidly pausing and unpausing to skip forward in frames)
                    if (currentTimeInMicroSec - mPauseTimeInMs > kTimePerFrameInMicroSec) {
                        numberOfFramesToProcess = 1;
                    }
                    else {
                        numberOfFramesToProcess = 0;
                        mLastUpdateTimeInMicroSec = currentTimeInMicroSec;
                        // Not perfect but assure not skipping frames at least
                    }
                }
                // Otherwise normal case: Calculate how many frames we should actually process based on real time that passed
                else {
                    uint64_t timePassedSinceLastFrameUpdate = currentTimeInMicroSec - mLastUpdateTimeInMicroSec;
                    uint64_t bigBoiNumOfFramesToProcess = timePassedSinceLastFrameUpdate / kTimePerFrameInMicroSec;

                    // Explicitly use a cast to quiet 64bit -> 32bit warnings.
                    // A wild world to live in if this cast was an issue
                    numberOfFramesToProcess = static_cast<FrameType>(bigBoiNumOfFramesToProcess);
                }

                if (numberOfFramesToProcess > 0) {
                    // Remember the exact timestamp we've accounted for (and no more or we can fall behind).
                    //      Note that we're doing this regardless of next bit of frames limitation code as we want the
                    //      time tracking to be as up to date as possible to prevent trying to process additional frames.
                    mLastUpdateTimeInMicroSec += kTimePerFrameInMicroSec * numberOfFramesToProcess;

                    // Limit max number of frames to process at once. Eg, for slow computes or when breakpoint debugging.
                    //      Especially cuz processing too many frames at once can be a death loop with slow computers and such
                    if (numberOfFramesToProcess > 3) { // Arbitrarily chosen, should rework in future!
                        numberOfFramesToProcess = 3; // Pretend to be caught up after this number of frames
                    }
                }
            }

            return numberOfFramesToProcess;
        }
    
      private:
        static constexpr uint64_t kTimePerFrameInMicroSec = static_cast<uint64_t>(FrameRate::TimePerFrameInMicroSec());
        
        std::function<uint64_t()> mTimeRetriever = []{ return SharedUtilities::getTimeInMicroseconds(); };
        uint64_t mLastUpdateTimeInMicroSec = 0;

        // Vars to properly handle pause + unpause
        bool mIsPaused = false;
        bool mShouldNextUpdateHandleUnpausing = false;
        uint64_t mPauseTimeInMs = 0;
    };
}
