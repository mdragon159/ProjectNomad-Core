#pragma once

#include "RollbackSettings.h"
#include "Input/CharacterInput.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    /**
    * Encapsulates storage of inputs for a single player during a rollback-enabled session.
    * This includes storing "confirmed" (not predicted) inputs for the player as well as retrieving input for frame
    * processing, regardless of whether it's predicted or confirmed.
    **/
    class RollbackPerPlayerInputs {
      public:
        bool SetupForNewSession(LoggerSingleton& logger,
                                const RollbackSettings& rollbackSettingso) {
            // Reset any other relevant vars
            mNextFrameToStore = 0;

            // Note that don't need to reset the ring buffer as unused frames are just "noise".
            // However, "head" value may be use for player predictions. This shouldn't matter in actual games, but
            // for edge case consistency always set the "head" value to a consistent default value.
            mConfirmedInputs.Add({});

            return true;
        }

        /**
        * Add the next "confirmed" (not predicted) input for a player
        * @param logger - Logger reference
        * @param targetFrame - Intended frame that trying to add input for. Used for validation purposes
        * @param input - "Confirmed" input for given player
        **/
        void AddInput(LoggerSingleton& logger,
                      FrameType targetFrame,
                      const CharacterInput& input) {
            // Sanity check: We should only be incrementally adding inputs for all players
            if (targetFrame != mNextFrameToStore) {
                logger.LogWarnMessage(
                    "Unexpected frame given! Expected next frame: " + std::to_string(targetFrame) +
                    ", provided input frame: " + std::to_string(targetFrame)
                );
                return;
            }

            mConfirmedInputs.Add(input); // Expectation: "Head" is always the last value we've received
            mNextFrameToStore++;
        }

        /**
        * Retrieves input to process the given target frame
        * @param logger - Logger reference
        * @param targetFrame - Target frame to retrieve player's input for
        * @returns Input to use for a player on the given frame. May be predicted or "confirmed" (actual) input
        **/
        const CharacterInput& GetInputForFrame(LoggerSingleton& logger, FrameType targetFrame) const {
            // Is target frame outside data that we've stored?
            if (targetFrame >= mNextFrameToStore) {
                // Valid input IF trying to retrieve input within prediction window
                if (!IsFrameOutsideOfGetRange(targetFrame)) {
                    return GetPredictedPlayerInput();
                }

                // Otherwise invalid situation:
                // Expecting calling code to add input for a frame before going too far
                logger.LogErrorMessage(
                    "Frame input greater than max frame stored! Received target frame " + std::to_string(targetFrame)
                    + " but next frame to store is " + std::to_string(mNextFrameToStore)
                );
                return mConfirmedInputs.Get(0); // Grab whatever is at head to ensure no out-of-bounds retrieval
            }
            
            // Simply look up the already stored value for that frame
            int offset = TargetFrameToLocalPlayerInputBufferOffset(logger, targetFrame);
            return mConfirmedInputs.Get(offset);
        }

        FrameType GetLastStoredFrame() const {
            return mNextFrameToStore - 1;
        }

        // Useful to confirm if missing too many inputs to process the next frame and thus should gameplay "delay" (freeze)
        bool IsFrameOutsideOfGetRange(FrameType targetFrame) const {
            return targetFrame > GetMaxPredictionFrame();
        }
        
      private:
        const CharacterInput& GetPredictedPlayerInput() const {
            // Always predict that player will use the latest known input.

            // Using this prediction as piggybacking off of typical FGC rollback algo findings: Using latest known input
            //  will be accurate more often than not as player isn't really switching inputs that fast compared to how
            //  fast simulation is.
            return mConfirmedInputs.Get(0);
        }

        FrameType GetMaxPredictionFrame() const {
            // No need to predict outside rollback window, as not supporting rollbacks at that point
            return mNextFrameToStore + RollbackStaticSettings::kMaxRollbackFrames - 1;
        }

        /**
        * Description
        * Few invariants are expected:
        * 1. If using positive InputDelay, then there are at least InputDelay inputs already stored before the "head"
        * 2. "Head" is the latest input stored from gameplay (due to style of ring buffer). This also means for
        *     frame 0, an input should be added BEFORE we try to retrieve. Which brings to next point...
        * 3. Already verified that targetFrame < mNextLocalFrameToStore
        * @param logger - Logger reference
        * @param targetFrame - desired frame to retrieve input from storage 
        * @returns offset to lookup desired input from relevant RingBuffer
        **/
        int TargetFrameToLocalPlayerInputBufferOffset(LoggerSingleton& logger, const FrameType targetFrame) const {
            FrameType offset = mNextFrameToStore - targetFrame - 1; // Should result in 0 when targetFrame is one less than next frame to store
            
            /// Sanity checks:
            // If target frame is outside intended window of inputs, then there's likely a higher level logic issue
            FrameType maxIntendedStoredInputs = RollbackStaticSettings::kMaxRollbackFrames + 1;
            if (offset > maxIntendedStoredInputs) {
                logger.LogWarnMessage(
                    "Trying to retrieve inputs outside expected range! Given target frame: " + std::to_string(targetFrame)
                    + ", offset: " + std::to_string(offset)
                    + ", maxIntendedStoredInputs: " + std::to_string(maxIntendedStoredInputs)
                );
                return 0;
            }
            // If target frame is outside max rollback buffer window entirely, then there's a very serious issue (out of bounds)
            if (offset > RollbackStaticSettings::kMaxRollbackFrames) {
                logger.LogWarnMessage(
                    "Offset is outside max buffer window! Target frame: " + std::to_string(targetFrame)
                    + ", offset: " + std::to_string(offset)
                );
                return 0;
            }

            // No issues with calculated offset so finally return it
            // Note that negative values represent the past in RingBuffer so flip the sign
            return static_cast<int>(offset) * -1;
        }

        // Storage for "confirmed" (not predicted) inputs. Head represents latest input given (ie, mNextFrameToStore - 1)
        RingBuffer<CharacterInput, RollbackStaticSettings::kOneMoreThanMaxRollbackFrames> mConfirmedInputs = {};
        FrameType mNextFrameToStore = 1000; // Starting session should set this back to 0. Cheap way for enforcing session start
    };
}
