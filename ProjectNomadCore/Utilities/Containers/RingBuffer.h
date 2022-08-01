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

            // Move head one spot forward then - primarily to be clean - make sure the head is pointing at a
            // valid index: [0, Size)
            mNextAddValueIndex = (mNextAddValueIndex + 1) % Size;
        }
        
        /**
        * Uses swap to insert the provided element into the "front" of the buffer
        * @param element - Element that is swap-inserted into the "front" of the buffer. No guarantee the new value
        *                   will be meaningful and thus new value should not be used after swap.
        **/
        void SwapInsert(ContentType& element) {
            std::swap(element, mArray[mNextAddValueIndex]);
            mNextAddValueIndex = (mNextAddValueIndex + 1) % Size;
        }

        /**
        * Uses swap to replace an existing stored value
        * @param offset - what spot to replace, relative to latest insertion. 0 = latest value, 1 = value before that, etc
        * @param element - what to replace currently stored value with via swap-replace. No guarantee the new value
        *                   will be meaningful and thus new value should not be used after swap.
        **/
        void SwapReplace(uint32_t offset, ContentType& element) {
            uint32_t index = CalculateIndex(offset);
            std::swap(element, mArray[index]);
        }

        /// <summary>
        /// Retrieves element at "front" (latest value) of buffer then moving "backwards" by offset amount.
        /// This call has no safeguards in case of an invalid offset and thus responsibility is fully on user to correctly
        /// use this method.
        /// </summary>
        /// <param name="offset">
        /// How many spots to get the value away from the current "front" of the buffer. Expected to be less than Size of buffer.
        /// Eg, supplying an offset of 2 will return the value inserted 3 elements ago (0th, 1st, 2nd spot).
        /// </param>
        /// <returns>Value stored in buffer represented by the "front" (latest inserted value) offsetted by the provided value</returns>
        const ContentType& Get(uint32_t offset) const {
            return mArray[CalculateIndex(offset)];
        }

      private:
        /**
        * Converts offset relative to latest insertion to internal array index
        * @param offset - How many spots to get the value away from the current "front" of the buffer. Expected to be
        *                 less than Size of buffer. Eg, supplying an offset of 2 will return the index representing
        *                 value inserted 3 elements ago (0th, 1st, 2nd spot).
        * @returns index represented by offset relative to latest insert position
        **/
        uint32_t CalculateIndex(uint32_t offset) const {
            // Multiple operations to calculate desired index
            //      1. Add Size to prevent underflow (assuming offset is < Size)
            //      2. Subtract 1 as head index is pointing one spot *ahead* of most recently added element
            //      3. Subtract offset to get to desired value
            //      4. Finally modulus Size to limit to index range: [0, Size)
            return (mNextAddValueIndex + Size - 1 - offset) % Size;
        }

        ContentType mArray[Size] = {};
        uint32_t mNextAddValueIndex = 0; // ie, "head"
    };
}
