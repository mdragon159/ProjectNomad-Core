#pragma once

#include <CRCpp/CRC.h>

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

      public:
        static constexpr uint32_t GetMaxSize() {
            return MaxSize;
        }

        uint32_t GetSize() const {
            return mHeadIndex;
        }

        bool IsEmpty() const {
            return mHeadIndex == 0;
        }

        // Returns true if succeeds
        bool Add(const ContentType& element) {
            if (mHeadIndex >= MaxSize) {
                return false;
            }
            
            mArray[mHeadIndex] = element;
            mHeadIndex++;
            return true;
        }

        /**
        * Adds all elements from another array to this array
        * @param other - Array to add elements from
        * @returns true if add succeeds, false otherwise
        **/
        bool AddAll(const FlexArray<ContentType, MaxSize>& other) {
            // Safety check to prevent going beyond max size
            if (GetSize() + other.GetSize() > MaxSize) {
                return false;
            }

            // Simply add each element one by one
            bool didAnyAddFail = false;
            for (uint32_t i = 0; i < other.GetSize(); i++) {
                if (!Add(other.Get(i))) {
                    didAnyAddFail = true;
                }
            }

            return !didAnyAddFail;
        }

        const ContentType& Get(uint32_t index) const {
            // Check index in bounds
            if (index > MaxSize - 1) {
                return mArray[0];
            }
            if (index >= mHeadIndex) {
                return mArray[0];
            }

            // Return desired element
            return mArray[index];
        }

        /**
        * Checks if array currently contains a given element
        * @param checkValue - Value to check for
        * @returns true if array contains element, false otherwise
        **/
        bool Contains(const ContentType& checkValue) const {
            // Simple linear search
            for (uint32_t i = 0; i < mHeadIndex; i++) {
                if (mArray[i] == checkValue) {
                    return true;
                }
            }
            
            return false;
        }

        /// <summary>
        /// Removes the element at the given index and moves last element to index.
        /// FUTURE: Supply iterator and erase functions. Consumer should not need to know to decrement index if looping and removing
        /// </summary>
        /// <returns>Returns true if succeeds in removing element at given index</returns>
        bool Remove(uint32_t index) {
            // Check index in bounds
            if (index > MaxSize - 1) {
                return false;
            }
            if (index >= mHeadIndex) {
                return false;
            }
            
            // If not at end, then move element at end into current spot (not retaining order so no need to move all elements around)
            if (index != mHeadIndex - 1) {
                // FUTURE: Swap may be faster...? At least in some cases?
                mArray[index] = mArray[mHeadIndex - 1];
            }
            mHeadIndex--; // Decrease active size of array by one; element pointed by head is now "invalid"/unused

            return true;
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&mHeadIndex, sizeof(mHeadIndex), CRC::CRC_32(), resultThusFar);

            // Check if ContentTyPpe has CalculateCRC32 method. From https://stackoverflow.com/a/22014784/3735890
            // This is vital as otherwise checksum will use padding bits. See BaseComponent.h comments for more info
            constexpr bool HasCalculateCRC32 = requires(const ContentType& element, uint32_t& result) {
                element.CalculateCRC32(result);
            };
            
            for (uint32_t i = 0; i < mHeadIndex; i++) {
                if constexpr (HasCalculateCRC32) {
                    mArray[i].CalculateCRC32(resultThusFar);
                }
                else {
                    resultThusFar = CRC::Calculate(&mArray[i], sizeof(mArray[i]), CRC::CRC_32(), resultThusFar);
                }
            }
        }

      private:
        ContentType mArray[MaxSize] = {};
        uint32_t mHeadIndex = 0; // Points to where next element should be added
    };
}
