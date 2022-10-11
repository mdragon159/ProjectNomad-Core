#pragma once

#include "Utilities/ILogger.h"

namespace ProjectNomad {
    /// <summary>
    /// Simple in-memory ring/circular buffer where "head" moves forward as each element is added, and older
    /// elements are overwritten as more elements get added.
    /// See the following for excellent visuals: https://en.wikipedia.org/wiki/Circular_buffer
    /// 
    /// In contrast to the wikipedia design with start/end or read/write heads, there's only one "end"/"write" head here.
    /// The expectation is that the user will use this buffer to store data every frame and then - when necessary -
    /// retrieve data a certain number of frames ago (within limit of Size).
    /// Eg, for game snapshots or player inputs within rollback limit.
    /// </summary>
    template <typename ContentType, uint32_t Size>
    class RingBuffer {
        static_assert(Size > 0, "MaxSize must be greater than 0");
      public:
        static constexpr uint32_t getSize() {
            return Size;
        }

        /**
        * Adds the given element to the "front" of the buffer
        * @param element - value to add to "front" of buffer
        **/
        void Add(const ContentType& element) {
            mArray[mNextAddValueIndex] = element;
            IncrementHead();
        }
        
        /**
        * Uses swap to insert the provided element into the "front" of the buffer
        * @param element - Element that is swap-inserted into the "front" of the buffer
        **/
        void SwapInsert(ContentType& element) {
            std::swap(element, mArray[mNextAddValueIndex]);
            IncrementHead();
        }

        /**
        * Uses swap to replace an existing stored value
        * @param offset - what spot to get, relative to latest insertion. 0 = latest value, -1 = second latest value,
        *                 1 = shortcut for value in other direction or "oldest value", etc.
        * @param element - what to replace currently stored value with via swap-replace
        **/
        void SwapReplace(int offset, ContentType& element) {
            uint32_t index = CalculateIndex(offset);
            std::swap(element, mArray[index]);
        }

        /**
        * Retrieves element at "front" (latest value) of buffer then moving "backwards" by offset amount.
        * @param offset - what spot to get, relative to latest insertion. 0 = latest value, -1 = second latest value,
        *                 1 = shortcut for value in other direction or "oldest value", etc.
        * @returns Value stored in buffer represented by the "front" (latest inserted value) offsetted by the provided value
        **/
        ContentType& Get(int offset) {
            return mArray[CalculateIndex(offset)];
        }

        // FUTURE: Remove redundant const call that's only used in old deprecated rollback-related code
        const ContentType& Get(int offset) const {
            return mArray[CalculateIndex(offset)];
        }

        /**
        * Moves head tracking one element forward without modifying any existing values
        **/
        void IncrementHead() {
            // Move head one spot forward then - primarily to be clean - make sure the head is pointing at a
            // valid index: [0, Size)
            mNextAddValueIndex = (mNextAddValueIndex + 1) % Size;
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&mNextAddValueIndex, sizeof(mNextAddValueIndex), CRC::CRC_32(), resultThusFar);

            // Check if ContentTyPpe has CalculateCRC32 method. From https://stackoverflow.com/a/22014784/3735890
            // This is vital as otherwise checksum will use padding bits. See BaseComponent.h comments for more info
            constexpr bool HasCalculateCRC32 = requires(const ContentType& element, uint32_t& result) {
                element.CalculateCRC32(result);
            };

            // Calculate checksum for buffer array. Note that there's no concept of "invalid" or unused values,
            // and thus must calculate checksum for all values.
            for (uint32_t i = 0; i < getSize(); i++) {
                if constexpr (HasCalculateCRC32) {
                    mArray[i].CalculateCRC32(resultThusFar);
                }
                else {
                    resultThusFar = CRC::Calculate(&mArray[i], sizeof(mArray[i]), CRC::CRC_32(), resultThusFar);
                }
            }
        }

      private:
        /**
        * Converts offset relative to latest insertion to internal array index
        * @param offset - How many spots to get the value away from the current "front" of the buffer. Expected to be
        *                 less than Size of buffer. Eg, supplying an offset of -2 will return the index representing
        *                 value inserted 3 elements ago (0th, 1st, 2nd spot).
        * @returns index represented by offset relative to latest insert position
        **/
        uint32_t CalculateIndex(int offset) const {
            // Multiple operations to calculate desired index
            //      1. Add Size to prevent underflow from negative value due to negative offset (assuming abs(offset) is < Size)
            //      2. Subtract 1 as head index is pointing one spot *ahead* of most recently added element
            //      3. Add offset to get to desired value
            //      4. Finally modulus Size to limit to index range: [0, Size)
            return (mNextAddValueIndex + Size - 1 + offset) % Size;
        }

        ContentType mArray[Size] = {};
        uint32_t mNextAddValueIndex = 0; // ie, "head"
    };
}
