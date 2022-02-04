#pragma once

#include "Utilities/ILogger.h"

namespace ProjectNomad {
    /// <summary>
    /// Array in place with fixed size but with helpful operations to have dynamic size in place (up to MaxSize).
    /// Intended to be alternative to std::vector for snapshot/memcpy behavior,
    ///     as std::vector's array is effectively a pointer to elsewhere in memory.
    /// NOTE: Order not guaranteed to be in add() call order after remove() is called. Check remove() implementation for more details
    /// </summary>
    template <typename ContentType, uint32_t MaxSize>
    class FlexArray {
        static_assert(MaxSize > 0, "MaxSize must be greater than 0");

        ContentType array[MaxSize];
        uint32_t headIndex = 0; // Points to where next element should be added

    public:
        static constexpr uint32_t getMaxSize() {
            return MaxSize;
        }

        uint32_t getSize() const {
            return headIndex;
        }

        bool isEmpty() const {
            return headIndex == 0;
        }

        // Returns true if succeeds
        bool add(const ContentType& element) {
            if (headIndex >= MaxSize) {
                return false;
            }
            
            array[headIndex] = element;
            headIndex++;
            return true;
        }

        const ContentType& get(uint32_t index) {
            // Check index in bounds
            if (index > MaxSize - 1) {
                return array[0];
            }
            if (index >= headIndex) {
                return array[0];
            }

            // Return desired element
            return array[index];
        }

        /// <summary>
        /// Removes the element at the given index and moves last element to index.
        /// FUTURE: Supply iterator and erase functions. Consumer should not need to know to decrement index if looping and removing
        /// </summary>
        /// <returns>Returns true if succeeds in removing element at given index</returns>
        bool remove(uint32_t index) {
            // Check index in bounds
            if (index > MaxSize - 1) {
                return false;
            }
            if (index >= headIndex) {
                return false;
            }
            
            // If not at end, then move element at end into current spot (not retaining order so no need to move all elements around)
            if (index != headIndex - 1) {
                // FUTURE: Swap may be faster...? At least in some cases?
                array[index] = array[headIndex - 1];
            }
            headIndex--; // Decrease active size of array by one; element pointed by head is now "invalid"/unused

            return true;
        }
    };
}
