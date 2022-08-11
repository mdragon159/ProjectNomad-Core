#pragma once
#include "GameCore/PlayerInput.h"
#include "Model/RollbackSessionInfo.h"
#include "Model/RollbackSettings.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    /**
    * This class encapsulates all input related logic for rollback related features, including input delay.
    * Note that this class is expected to use in-place memory (eg, NOT std::vectors) to support memcpy for direct
    * snapshot creation and restoration.
    **/
    class RollbackInputManager {
      public:
        /**
        * Start a new session. Expected to be called before any other methods are used.
        * @param rollbackSettings - Validated rollback settings to use
        * @param rollbackSessionInfo - Validated session settings to use
        **/
        void OnSessionStart(const RollbackSettings& rollbackSettings, const RollbackSessionInfo& rollbackSessionInfo) {
            // Handle setting up input delay as necessary (eg, pre-fill inputs for initial delay)
            SetupInputDelay(rollbackSettings);
            
            // Reset any other relevant vars
            mNextLocalFrameToStore = 0;
        }

        /**
        * Adds player input for later retrieval. Expected to be called for a given frame BEFORE calling the respective Get.
        *
        * Note that (negative and positive) input delay is handled by this manager and thus it is NOT guaranteed that
        * calling Get with the given target frame will return the same value.
        * @param targetFrame - current frame from caller's perspective to add input for
        * @param localPlayerInput - the input to store
        **/
        void AddLocalPlayerInput(FrameType targetFrame, const PlayerInput& localPlayerInput) {
            if (mIsUsingLocalNegativeInputDelay) {
                // If initial prediction frames, then *don't* add any input as using default inputs for these frames
                if (targetFrame + 1 < mNegativeLocalInputDelay) {
                    return;
                }
                
                // Otherwise offset target frame for prediction frames as consuming code's simulation is technically
                // AHEAD of stored inputs (the "source of truth" for player inputs).
                // ie, help assure internal storage state only represents validated inputs from OUTSIDE prediction frames
                targetFrame = targetFrame + 1 - mNegativeLocalInputDelay;
            }
            
            // Sanity check: We should only be incrementally adding inputs for the local player
            if (targetFrame != mNextLocalFrameToStore) {
                Singleton<LoggerSingleton>::get().logErrorMessage(
                    "RollbackInputManager::AddLocalPlayerInput",
                    "Bad frame input! Received target frame " + std::to_string(targetFrame)
                    + " but mNextFrameToStore is " + std::to_string(mNextLocalFrameToStore)
                );
                return;
            }

            mLocalPlayerInputs.Add(localPlayerInput); // Expectation: "Head" is always the last value we've received
            mNextLocalFrameToStore++;
        }

        /**
        * Retrieves input for local player for given frame based on what's currently known.
        * NOTE: 
        * @param targetFrame - the frame to retrieve input for. This should be
        * @returns "appropriate" input for given frame based on what was previously stored and input delay settings
        **/
        const PlayerInput& GetLocalPlayerInput(const FrameType targetFrame) {
            // Is target frame outside data that we've stored?
            if (targetFrame >= mNextLocalFrameToStore) {
                // If using negative input delay, then valid input IF trying to retrieve input from within prediction window
                if (mIsUsingLocalNegativeInputDelay && targetFrame <= GetMaxPredictionFrame()) {
                    return GetPredictedLocalPlayerInput();
                }
                
                // Otherwise invalid situation:
                // Expecting calling code to always add input for a frame before trying to retrieve it
                Singleton<LoggerSingleton>::get().logErrorMessage(
                    "RollbackInputManager::GetLocalPlayerInput",
                    "Bad frame input! Received target frame " + std::to_string(targetFrame)
                    + " but mNextFrameToStore is " + std::to_string(mNextLocalFrameToStore)
                );
                return mLocalPlayerInputs.Get(0); // Grab whatever is at head to ensure no out-of-bounds retrieval
            }

            // Simply look up the already stored value for that frame
            return mLocalPlayerInputs.Get(TargetFrameToLocalPlayerInputBufferOffset(targetFrame));
        }
         
        /**
        * Retrieves prediction used for the provided target frame.
        * WARNING: Calling the add input operation will overwrite what this returns! Should call this BEFORE storing new input
        * @param targetFrame - "real" frame (input delay adjusted) to retrieve input for
        * @returns predicted input used for target frame.
        *          NOT returning by const reference as theoretically enough writes would modify this value. Shouldn't
        *          be necessary but nice to not have to think about the underlying data storage's limitations.
        **/
        PlayerInput GetLocalPlayerPredictedInputForFrame(const FrameType targetFrame) const {
            // Note that - with current prediction logic - we reuse the same prediction for ALL prediction frames
            // (until a new input is provided).
            //
            // Thus, we don't actually need the frame input parameter BUT we define it anyways in order to mask
            // implementation from caller. After all, prediction logic may change in future and would be nice to not
            // have to update callers too.
            return GetPredictedLocalPlayerInput();
        }

      private:
        void SetupInputDelay(const RollbackSettings& rollbackSettings) {
            mIsUsingLocalNegativeInputDelay = rollbackSettings.localInputDelay < 0;
            mPositiveLocalInputDelay =
                mIsUsingLocalNegativeInputDelay ?  0 : static_cast<FrameType>(rollbackSettings.localInputDelay);
            
            if (mIsUsingLocalNegativeInputDelay) {
                mIsUsingLocalNegativeInputDelay = true;
                mNegativeLocalInputDelay = static_cast<FrameType>(rollbackSettings.localInputDelay * -1);

                // Given "prediction" is just using latest actual input, add a default value to start with
                // (If this is NOT done, then first predictions will be undefined/will vary depending on prior state)
                mLocalPlayerInputs.Add({});
            }
            // Using positive input delay...?
            else if (rollbackSettings.localInputDelay > 0) {
                /** Prefill inputs RingBuffer with enough inputs to account for delay!
                * Why necessary? Sample scenario: Have 3 frames of input delay but want input for frame 0.
                * We'll store the actual input from the player for frame 3, but also need to deterministically define
                * the input later returned for that gameplay frame 0 processing
                */
                for (FrameType i = 0; i < mPositiveLocalInputDelay; i++) {
                    mLocalPlayerInputs.Add({}); // Use "empty"/default inputs for initial delayed frames
                }
            }
        }

        const PlayerInput& GetPredictedLocalPlayerInput() const {
            // Always predict that player will use the latest known input.

            // Using this prediction as piggybacking off of typical FGC rollback algo findings: Using latest known input
            //  will be accurate more often than not as player isn't really switching inputs that fast compared to how
            //  fast simulation is.
            return mLocalPlayerInputs.Get(0);
        }

        FrameType GetMaxPredictionFrame() const {
            // Eg, if at start of game (mNextLocalFrameToStore = 0) with 3 frames of negative input delay,
            // then should be predicting from frames 0 to 2
            return mNextLocalFrameToStore + mNegativeLocalInputDelay - 1;
        }

        /**
        * Description
        * Few invariants are expected:
        * 1. If using positive InputDelay, then there are at least InputDelay inputs already stored before the "head"
        * 2. "Head" is the latest input stored from gameplay (due to style of ring buffer). This also means for
        *     frame 0, an input should be added BEFORE we try to retrieve. Which brings to next point...
        * 3. Already verified that targetFrame < mNextLocalFrameToStore
        * @param targetFrame - desired frame to retrieve input from storage 
        * @returns offset to lookup desired input from relevant RingBuffer
        **/
        uint32_t TargetFrameToLocalPlayerInputBufferOffset(const FrameType targetFrame) {
            // 

            FrameType offsetWithoutInputDelay = mNextLocalFrameToStore - targetFrame - 1; // Should result in 0 when targetFrame is one less than next frame to store
            FrameType offset = offsetWithoutInputDelay + mPositiveLocalInputDelay;
            
            /// Sanity checks:
            // If target frame is outside intended window of inputs, then there's likely a higher level logic issue
            FrameType maxIntendedStoredInputs = RollbackStaticSettings::kMaxRollbackFrames + mPositiveLocalInputDelay + 1;
            if (offset > maxIntendedStoredInputs) {
                Singleton<LoggerSingleton>::get().logWarnMessage(
                    "RollbackInputManager::TargetFrameToStoredInputBufferOffset",
                    "Trying to retrieve inputs outside expected range! Given target frame: " + std::to_string(targetFrame)
                    + ", offset: " + std::to_string(offset)
                    + ", maxIntendedStoredInputs: " + std::to_string(maxIntendedStoredInputs)
                );
            }
            // If target frame is outside max rollback buffer window entirely, then there's a very serious issue (out of bounds)
            if (offset > RollbackStaticSettings::kMaxBufferWindow) {
                Singleton<LoggerSingleton>::get().logWarnMessage(
                    "RollbackInputManager::TargetFrameToStoredInputBufferOffset",
                    "Offset is outside max buffer window! Target frame: " + std::to_string(targetFrame)
                    + ", offset: " + std::to_string(offset)
                );
                return 0;
            }

            // No issues with calculated offset so finally return it
            return offset;
        }
        
        RingBuffer<PlayerInput, RollbackStaticSettings::kMaxBufferWindow> mLocalPlayerInputs;
        FrameType mNextLocalFrameToStore = 1000; // Starting session should set this back to 0. Cheap way for enforcing session start
        
        // Input delay related vars
        // Note that using separate pos vs negative input delay vars to improve readability via simplifying math/conditionals
        bool mIsUsingLocalNegativeInputDelay = false;
        FrameType mPositiveLocalInputDelay = 0;
        FrameType mNegativeLocalInputDelay = 0;

        // TODO: Combine complexity of RingBuffer + mNextLocalFrameToStore + mPositiveLocalInputDelay into its own class
        //       as there's a whole lotta logic here just to encapsulate those relevant expectations.
        //       ie, encapsulate the "source of truth" for inputs into its own class.
    };
}
