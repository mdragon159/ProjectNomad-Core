#pragma once

#include "Context/FrameRate.h"
#include "Rollback/Model/RollbackSettings.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/SharedUtilities.h"

namespace ProjectNomad {
    /**
    * Encapsulates responsibility for appropriate timing of gameplay.
    **/
    class RollbackTimeManager {
      public:
        RollbackTimeManager() = default;
        // Special constructor for unit tests so can easily, quickly, and precisely test behavior
        explicit RollbackTimeManager(std::function<uint64_t()> timeRetriever) : mTimeRetriever(std::move(timeRetriever)) {}
        
        void Start() {
            // Reset state back to defaults (but retain any values passed in via constructors)
            auto timeRetrieverCopy = mTimeRetriever;
            *this = {}; // Reset all values to their defaults, which are expected to be valid for startup usage
            mTimeRetriever = timeRetrieverCopy;
        }

        bool IsPaused() const {
            return mIsPaused;
        }
        void Pause() {
            mIsPaused = true;
            mShouldNextUpdateHandleUnpausing = false; // Just in case, as this is our expectation anyways
            mPauseTimeInMicroSec = mTimeRetriever();
        }
        void Resume() {
            mIsPaused = false;
            mShouldNextUpdateHandleUnpausing = true;
        }

        /**
        * Sets up "time sync" functionality between self and host.
        * 
        * This is vital as otherwise there is no mechanic to close time drift/gap between players. For example, if
        * two players are at max rollback frames apart and are stalling, then that stalling/delay may never stop without
        * this time sync functionality.
        *
        * Thus, the goal here is to take the input (representing the current time drift) and try to get that roughly
        * close to 0.
        * In addition, we want to spread this time sync over a gradual amount of time so player doesn't get jarring
        * speed up or slow downs during their play.
        * @param logger - Logger reference
        * @param hostNumberOfFramesAhead - What's the frame difference between host and self? (Host frame - local frame)
        *                                  Expected to be 0 if self is host (as all other players are trying to close
        *                                  gap with host, which may be from different directions at once).
        **/
        void SetupTimeSyncForRemoteFrameDifference(LoggerSingleton& logger, const int64_t hostNumberOfFramesAhead) {
            const int64_t unsignedFrameDifference = std::abs(hostNumberOfFramesAhead);
            // Sanity check input to help quickly catch any potential bugs during runtime.
            //      NOTE: Theoretically we should never be able to be more than max rollback frames apart.
            //            However, messages take a while to go back and forth. Need to see if this could ever realistically happen. 
            if (unsignedFrameDifference > RollbackStaticSettings::kMaxRollbackFrames) { // Should never be able to be more than max rollback frames apart
                logger.LogWarnMessage(
                    "Input out of expected range, is this a valid case? Max rollback frames: " +
                    std::to_string(RollbackStaticSettings::kMaxRollbackFrames) + ", signed input: " +
                    std::to_string(hostNumberOfFramesAhead)
                );
                // Don't immediately exit out as this *may* theoretically be a valid case.
            }
            // If frame advantage/difference is within the desired threshold, then nothing to do.
            //      See comments on threshold variable for more info. In short, this prevents time sync overshoots.
            if (unsignedFrameDifference <= kTimeSyncFrameDifferenceThreshold) {
                logger.LogWarnMessage("Resetting time sync as below threshold with frame difference of: " + std::to_string(hostNumberOfFramesAhead));

                ResetTimeSyncStatus(); // In case there was any prior time sync handling going on, which is now unnecessary
                return;
            }

            // Set cooldown to track how long we should be using this time multiplier.
            //      Note that we could just try to use the calculated time multiplier until we receive an update that
            //      time drift has been closed "enough". However, we're more likely than not to get that update message
            //      relatively late (given delay in sending messages back and forth). Thus, safest to track our own
            //      limit on the duration of this time sync attempt
            mTimeSyncRemainingDuration = kTimeSyncDuration;
            
            // Calculate new time multiplier based on how fast we need to go to close that gap over pre-defined amount of time.
            //      Eg, if gameplay normally runs at 60fps, host is +10 frames, and want to get rid of that gap in 3 seconds,
            //          then 10/3 = should run +3.333 fps faster. OR run at 63.333fps total.
            float frameDifferencePerSecond = hostNumberOfFramesAhead / kHowLongShouldTimeSyncTakeInSec;
            // Note that input param is signed. So if it's positive (signifying we're behind the host), then
            //      we should speed up while vice versa if negative.
            float newFrameRatePerSecond = frameDifferencePerSecond + FrameRate::kGameplayFrameRate;
            mTimeSyncTimeMultiplier = newFrameRatePerSecond / FrameRate::kGameplayFrameRate; // Store multiplier instead of raw frame rate as this feels mentally easier to think about and use.
            
            // Clamp time multiplier within max desired speed difference range to assure no jarring experience for player.
            //  Eg, if range == 0.1, then max 10% speed difference (whether slowing down or speeding up)
            mTimeSyncTimeMultiplier = std::clamp(mTimeSyncTimeMultiplier, 1 - kMaxTimeMultiplierRange, 1 + kMaxTimeMultiplierRange);

            logger.LogInfoMessage("Setting time sync up for hostNumberOfFramesAhead: " +
                std::to_string(hostNumberOfFramesAhead) + " with time multiplier: " + std::to_string(mTimeSyncTimeMultiplier));
        }

        /**
        * Calculates how many gameplay frames need to be handled in order to maintain desired fps simulation
        * @returns number of gameplay frames that need to be processed to maintain desired fps simulation
        **/
        FrameType CheckHowManyFramesToProcess() {
            if (IsPaused()) {
                return 0;
            }
            
            const uint64_t currentTimeInMicroSec = mTimeRetriever();

            // Special case: Is this the first update call? Need special handling as timer may not yet be setup.
            //      Could also check if tracked last update time == 0, but gets a bit tricky for unit testing.
            //      More "sane" and thus robust this way, as not expanding on meaning of variable at different values.
            if (!mHandledInitialFrameProcessing) {
                mHandledInitialFrameProcessing = true;

                mLastUpdateTimeInMicroSec = currentTimeInMicroSec; // Prepare for next run
                return 1; // Do necessary work for first frame
            }
            // Special unpausing case: Make sure to only handle 1 frame max
            if (mShouldNextUpdateHandleUnpausing) {
                mShouldNextUpdateHandleUnpausing = false; // Remember that frame processing for unpause already handled

                // If at least 1 frame's worth of time passed, then only process a single frame when unpausing.
                //      (ie, prevent being able to "rapidly" skip forward by rapidly pausing and unpausing)
                if (currentTimeInMicroSec - mPauseTimeInMicroSec > kTimePerFrameInMicroSec) {
                    // Update current time so don't try to make up for those other frames during pause.
                    //      Not the most perfect approach as may have been cusp on processing another frame, but
                    //      good enough atm.
                    mLastUpdateTimeInMicroSec = currentTimeInMicroSec;
                    
                    return 1;
                }

                return 0; // Otherwise not enough time passed while paused to process any frames
            }
            
            // Standard time handling case
            return GetFramesToProcessBasedOnStandardTimePassing(currentTimeInMicroSec);
        }

        // Expose this setting for easy unit testing. Perhaps would be cleaner to refactor out to a helper class or such?
        static constexpr FrameType GetMaxFramesPossibleToProcessAtOnce() {
            return kMaxFramesToProcessAtOnce;
        }
    
      private:
        FrameType GetFramesToProcessBasedOnStandardTimePassing(const uint64_t currentTimeInMicroSec) {
            // Time multiplier: Adjusting expected length of frame will adjust how fast or slow time passes by
            const uint64_t curTimePerFrameInMicroSec = GetAdjustedTimePerFrameInMicroSec();
            
            // Calculate how many frames we should actually process based on real time that passed
            uint64_t timePassedSinceLastFrameUpdate = currentTimeInMicroSec - mLastUpdateTimeInMicroSec;
            uint64_t bigBoiNumOfFramesToProcess = timePassedSinceLastFrameUpdate / curTimePerFrameInMicroSec;
            // Explicitly use a cast to quiet 64bit -> 32bit warnings.
            //      A wild world to live in if this cast was an issue (as calculated based on time displacement).
            FrameType numberOfFramesToProcess = static_cast<FrameType>(bigBoiNumOfFramesToProcess);

            if (numberOfFramesToProcess > 0) {
                // Remember the exact timestamp we've accounted for (and no more or we can fall behind).
                //      Note that we're doing this regardless of next bit of frames limitation code as we want the
                //      time tracking to be as up to date as possible to prevent trying to process additional frames.
                mLastUpdateTimeInMicroSec += curTimePerFrameInMicroSec * numberOfFramesToProcess;

                // Limit max number of frames to process at once. Eg, for slow computes or when breakpoint debugging.
                //      Especially cuz processing too many frames at once can be a death loop with slow computers and such
                if (numberOfFramesToProcess > kMaxFramesToProcessAtOnce) {
                    numberOfFramesToProcess = kMaxFramesToProcessAtOnce; // Pretend to be caught up after this number of frames
                }
            }

            ProcessTimeSyncDuration(numberOfFramesToProcess);
            return numberOfFramesToProcess;
        }

        uint64_t GetAdjustedTimePerFrameInMicroSec() const {
            /*
             * Ironically, this is probably the one place we can use floats as exact accuracy doesn't matter at all BUT
             * we can easily overflow here if we use our standard fixed point (which are sort of "truncated" signed 64bit values).
             *
             * Why exact accuracy doesn't matter:
             *      Because this "time adjustment" is *specifically* used for closing in on time drift between players.
             *      We're not expecting this process between two very different remote computers to be exact, but rather
             *      we're trying to close the gap "gradually", check where the gap is, then continue until we hit a certain
             *      gap threshold to stop.
             *
             *      And given the time adjustment rate shouldn't be too "extreme" (such as not 50% speed but perhaps 95%
             *      speed over several seconds), a small exact variance shouldn't matter.
             *
             *      After all, a small error in "time adjustment" won't cause a full rollback desync.
             */

            // TODO: Unit test the heck outta this. Eg, if time per frame in microsec is VERY high value, then result
            //       should not be that off

            return mTimeSyncTimeMultiplier * kTimePerFrameInMicroSec;
        }

        void ProcessTimeSyncDuration(const FrameType numberOfFramesToProcess) {
            // Nothing to do if not using time sync functionality OR not actually trying to process any frames yet
            if (mTimeSyncRemainingDuration == 0 || numberOfFramesToProcess == 0) {
                return;
            }

            // Reset case: Is desired time sync period over now?
            if (numberOfFramesToProcess >= mTimeSyncRemainingDuration) { // Subtraction underflow protection check
                ResetTimeSyncStatus();
            }
            else {
                // Minor TODO: Note that this does not take into account if TimeManager user doesn't actually use these
                //             frames (like during stalls). Expecting this to not really matter much, but should test
                //             thoroughly via inducing stalls under different scenarios
                mTimeSyncRemainingDuration -= numberOfFramesToProcess;
            }
        }

        void ResetTimeSyncStatus() {
            mTimeSyncRemainingDuration = 0;
            mTimeSyncTimeMultiplier = 1;
        }
        
        static constexpr uint64_t kTimePerFrameInMicroSec = static_cast<uint64_t>(FrameRate::TimePerFrameInMicroSec());
        static constexpr FrameType kMaxFramesToProcessAtOnce = 3; // Arbitrarily chosen, should rework in future!
        
        std::function<uint64_t()> mTimeRetriever = []{ return SharedUtilities::getTimeInMicroseconds(); };
        uint64_t mLastUpdateTimeInMicroSec = 0;
        bool mHandledInitialFrameProcessing = false; // Special start case, as timer state may not be set correctly then

        // Vars to properly handle pause + unpause
        bool mIsPaused = false;
        bool mShouldNextUpdateHandleUnpausing = false;
        uint64_t mPauseTimeInMicroSec = 0;

        /// Time Sync variables below

        // When should we stop trying to reduce time drift with remote player (host)?
        // Must be *greater* than threshold to activate time sync functionality.
        //    Note that this is necessary as otherwise we may constantly try to close time drift at all times, which would
        //    cause a constant overshoot effect (like a perfect spring constantly jumping past the rest point).
        static constexpr FrameType kTimeSyncFrameDifferenceThreshold = 1; // Arbitrarily chosen as want to get as close to 0 as possible

        static constexpr float kMaxTimeMultiplierRange = 0.1f; // Max 10% speed up or slow down should be a good guesstimate boundary to start with
        // Define a duration to try to fix entire time drift, which *should* be a decent balance of time sync speed vs
        //      minimizing player impact from speed changes.
        //      See above rate limitation, which preferably shouldn't be hit at all. Otherwise may suffer from inaccuracies.
        //      FUTURE: Perhaps do some static asserts to assure max time multiplier range won't get hit if input within expected range (kMaxRollbackFrames)?
        static constexpr float kHowLongShouldTimeSyncTakeInSec = 3; // Based on a mentioned time in GGPO discord from memory
        static constexpr FrameType kTimeSyncDuration = FrameRate::FromSeconds(fp(kHowLongShouldTimeSyncTakeInSec));
        
        float mTimeSyncTimeMultiplier = 1; // Why using floats? See comments in GetAdjustedTimePerFrameInMicroSec()
        FrameType mTimeSyncRemainingDuration = 0; // How long until we should stop using this time multiplier?
    };
}
