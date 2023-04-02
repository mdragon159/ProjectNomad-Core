#pragma once

#include <array>
#include <CRCpp/CRC.h>

#include "InputCommand.h"
#include "CharacterInput.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    /**
    * Utility class which remembers necessary data for "proper" (as intended) input buffer behavior of a single input.
    *
    * Specifically, this supports the following:
    * - Remembers if input used in a given frame so can later clear it. (So one press == one activation)
    * - Remembers when input given so can clear it if too long passes without being used. (So limited buffer time)
    *
    * Note that there's no circular buffer in CommandInputBuffer or here as NOT supporting the same command being buffered
    * multiple times. Can't see a need for it atm and may even get in way of intended behavior for player (eg, rapidly spamming
    * jump but only meaning to jump once)
    **/
    class BufferedInputData {
      public:
        // GetWithoutConsumingInput + MarkConsumed support checking without consuming for a bit of flexibility with coding styles
        bool GetWithoutConsumingInput() const {
            return mIsSet;
        }
        void MarkConsumed() {
            mWasUsed = true;
        }
        // This is intended to support a single input not, say, re-toggling an action instantly
        // Eg, pressing crouch once should not go into crouch state then exit crouch state in same frame regardless of
        // order of relevant systems/logic processing
        void ImmediatelyResetInputPress() {
            mIsSet = false;
        }
        
        // Likely preferred way to get inputs for actions, as calling code doesn't need to worry about a second call
        bool GetAndConsumeInput() {
            mWasUsed = true;
            return mIsSet;
        }

        void RememberInputSet(FrameType curFrame) {
            mIsSet = true;

            mSetFrame = curFrame;
            mWasUsed = false;
        }

        /**
        * Clears any inputs which have been already used OR were stored too long ago in the past
        * @param latestCompletedFrame - Latest frame which has already finished using this buffer. Note that this is
        *                               expected to be safe to call at beginning of game with FrameType's max value.
        **/
        void ClearIfConsumedOrExpired(FrameType latestCompletedFrame) {
            if (!mIsSet) { // Nothing to do if input not even set
                return;
            }

            if (mWasUsed || latestCompletedFrame - mSetFrame >= kBufferedInputLifetime) {
                mIsSet = false;
            }
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&mSetFrame, sizeof(mSetFrame), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&mIsSet, sizeof(mIsSet), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&mWasUsed, sizeof(mWasUsed), CRC::CRC_32(), resultThusFar);
        }

      private:
        static constexpr FrameType kBufferedInputLifetime = 7; // an input should only be remembered for this long
        
        FrameType mSetFrame = 0;
        bool mIsSet = false;
        bool mWasUsed = false; // Remember if input was used so we can clear it from the buffer (eg, at end of frame)
    };
}
