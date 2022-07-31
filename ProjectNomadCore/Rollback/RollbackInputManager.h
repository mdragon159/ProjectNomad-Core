#pragma once
#include "GameCore/PlayerInput.h"
#include "Model/RollbackSessionInfo.h"
#include "Model/RollbackSettings.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    class RollbackInputManager {
      public:
        void OnSessionStart(const RollbackSettings& rollbackSettings, const RollbackSessionInfo& rollbackSessionInfo) {
            // Handle setting up input delay as necessary (eg, pre-fill inputs for initial delay)
            SetupInputDelay(rollbackSettings);
            
            // Reset any other relevant vars
            mNextLocalFrameToStore = 0;
        }

        void AddLocalPlayerInput(const FrameType targetFrame, const PlayerInput& localPlayerInput) {
            // Sanity check: We should be only incrementally adding inputs for the local player
            if (targetFrame != mNextLocalFrameToStore) {
                mLogger.logErrorMessage(
                    "RollbackInputManager::AddLocalPlayerInput",
                    "Bad frame input! Received target frame " + std::to_string(targetFrame)
                    + " but mNextFrameToStore is " + std::to_string(mNextLocalFrameToStore)
                );
                return;
            }

            mLocalPlayerInputs.add(localPlayerInput); // Expectation: "Head" is always the last value we've received
            mNextLocalFrameToStore++;
        }
        
        const PlayerInput& GetLocalPlayerInput(const FrameType targetFrame) {
            // Sanity check: Expecting calling code to always add input for a frame before trying to retrieve it
            if (targetFrame >= mNextLocalFrameToStore) {
                mLogger.logErrorMessage(
                    "RollbackInputManager::GetLocalPlayerInput",
                    "Bad frame input! Received target frame " + std::to_string(targetFrame)
                    + " but mNextFrameToStore is " + std::to_string(mNextLocalFrameToStore)
                );
                return mLocalPlayerInputs.get(0); // Grab whatever is at head to ensure no out-of-bounds retrieval
            }

            // Otherwise, assuming no negative input delay (unlikely case), simply look up the stored input
            if (!mIsUsingLocalNegativeInputDelay) {
                return mLocalPlayerInputs.get(TargetFrameToStoredInputBufferOffset(targetFrame));
            }

            // Fun negative input delay case: Predict the future frame's input! 
            return GetPredictedLocalPlayerInput(targetFrame);
        }

      private:
        void SetupInputDelay(const RollbackSettings& rollbackSettings) {
            mIsUsingLocalNegativeInputDelay = rollbackSettings.localInputDelay < 0;
            mPositiveLocalInputDelay =
                mIsUsingLocalNegativeInputDelay ?  0 : static_cast<FrameType>(rollbackSettings.localInputDelay);
            
            if (mIsUsingLocalNegativeInputDelay) {
                mIsUsingLocalNegativeInputDelay = true;
                mNegativeLocalInputDelay = static_cast<FrameType>(rollbackSettings.localInputDelay * -1);
                // TODO: Have input prediction in its own list...?
            }
            // Using positive input delay...?
            else if (rollbackSettings.localInputDelay > 0) {
                /** Prefill inputs RingBuffer with enough inputs to account for delay!
                * Why necessary? Sample scenario: Have 3 frames of input delay but want input for frame 0.
                * We'll store the actual input from the player for frame 3, but also need to deterministically define
                * the input later returned for that gameplay frame 0 processing
                */
                for (FrameType i = 0; i < mPositiveLocalInputDelay; i++) {
                    mLocalPlayerInputs.add({}); // Use "empty"/default inputs for initial delayed frames
                }
            }
        }

        const PlayerInput& GetPredictedLocalPlayerInput(const FrameType targetFrame) {
            // TODO: Use input prediction!
            // BUT: What if the input is NOT in negative input delay space?
            // eg, negative input delay = 3 frames. Gameplay is at frame 5 (so stored up to frame 5 worth of inputs)
            //          BUT we're currently rolling back and want the known input for frame 1.
            mLogger.logErrorMessage("GetLocalPlayerInput", "Not yet implemented negative input delay retrieval!");
            return mLocalPlayerInputs.get(0);
        }

        uint32_t TargetFrameToStoredInputBufferOffset(const FrameType targetFrame) {
            // Few invariants are expected:
            // 1. If using positive InputDelay, then there are at least InputDelay inputs already stored before the "head"
            // 2. "Head" is the latest input stored from gameplay (due to style of ring buffer). This also means for
            //      frame 0, an input should be added BEFORE we try to retrieve. Which brings to next point...
            // 3. Already verified that targetFrame < mNextLocalFrameToStore

            FrameType offsetWithoutInputDelay = mNextLocalFrameToStore - targetFrame - 1; // Should result in 0 when targetFrame is one less than next frame to store
            FrameType offset = offsetWithoutInputDelay + mPositiveLocalInputDelay;
            
            /// Sanity checks:
            // If target frame is outside intended window of inputs, then there's likely a higher level logic issue
            FrameType maxIntendedStoredInputs = RollbackStaticSettings::kMaxRollbackFrames + mPositiveLocalInputDelay + 1;
            if (offset > maxIntendedStoredInputs) {
                mLogger.logWarnMessage(
                    "RollbackInputManager::TargetFrameToStoredInputBufferOffset",
                    "Trying to retrieve inputs outside expected range! Given target frame: " + std::to_string(targetFrame)
                    + ", offset: " + std::to_string(offset)
                    + ", maxIntendedStoredInputs: " + std::to_string(maxIntendedStoredInputs)
                );
            }
            // If target frame is outside max rollback buffer window entirely, then there's a very serious issue (out of bounds)
            if (offset > RollbackStaticSettings::kMaxBufferWindow) {
                mLogger.logWarnMessage(
                    "RollbackInputManager::TargetFrameToStoredInputBufferOffset",
                    "Offset is outside max buffer window! Target frame: " + std::to_string(targetFrame)
                    + ", offset: " + std::to_string(offset)
                );
                return 0;
            }

            // No issues with calculated offset so finally return it
            return offset;
        }
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();

        // Input delay related vars
        // Note that using separate pos vs negative input delay vars to improve readability via simplifying math/conditionals
        bool mIsUsingLocalNegativeInputDelay = false;
        FrameType mPositiveLocalInputDelay = 0;
        FrameType mNegativeLocalInputDelay = 0;

        FrameType mNextLocalFrameToStore = 1000; // Starting session should set this back to 0. Cheap way for enforcing session start
        RingBuffer<PlayerInput, RollbackStaticSettings::kMaxBufferWindow> mLocalPlayerInputs;
        // TODO: Tracking var for translating where "retrieval head" is compared to RingBuffer head
    };
}
