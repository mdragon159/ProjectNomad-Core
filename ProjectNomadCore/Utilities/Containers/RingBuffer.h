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

        ContentType array[Size];
        uint32_t headIndex = 0; // Points to where next element should be added

    public:
        RingBuffer() {}
        
        static constexpr uint32_t getSize() {
            return Size;
        }

        /// <summary>Adds the given element to the "front" of the buffer</summary>
        void add(const ContentType& element) {
            array[headIndex] = element;

            // Move head one spot forward then - primarily to be clean - make sure the head is pointing at a valid index: [0, Size)
            headIndex = (headIndex + 1) % Size;
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
        const ContentType& get(uint32_t offset) {
            // Multiple operations to calculate desired index
            //      1. Add Size to prevent underflow (assuming offset is < Size)
            //      2. Subtract 1 as headIndex is pointing one spot *ahead* of most recently added element
            //      3. Subtract offset to get to desired value
            //      4. Finally modulus Size to limit to index range: [0, Size)
            uint32_t index = (headIndex + Size - 1 - offset) % Size;

            return array[index];
        }
    };
}
