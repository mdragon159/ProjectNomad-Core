#pragma once

#include <type_traits>
#include <CRCpp/CRC.h>

namespace ProjectNomad {
    /**
    * Bitset implementation with a well-defined numeric type under the hood.
    * Essentially like std::bitset or a array of bools, but with a fixed numeric representation under the hood for
    *       guaranteed easy serialization and deserialization.
    * TODO: Unit tests! Currently NO unit tests specifically covering this pure utility container class!
    * 
    *  Made this as std::array, std::bitset, etc don't make contiguous memory guarantees. Also tried to use std::array<bool>
    *        with a size of 32 and got "incorrect" results when casting first spot's memory to uint32_t.
    * @tparam BaseType - Unsigned integral type to use as internal numeric representation
    **/
    template <typename BaseType>
    class NumericBitSet {
        static_assert(std::is_integral_v<BaseType>, "BaseType must be an integral type");
        static_assert(std::is_unsigned_v<BaseType>, "BaseType is expected to be an unsigned type for extra caution"); // May use little tricks to convert to bool or such
      public:
        NumericBitSet() = default;
        explicit NumericBitSet(BaseType initialValue) : mInternalRepresentation(initialValue) {}

        /**
        * Retrieves the bit flag in the corresponding spot.
        * Bit manipulation methods from https://stackoverflow.com/a/47990/3735890
        * @param index - Index to set value for. Index starts at 0 and must be less than number of bits in BaseType.
        *                   "Undefined" behavior if input breaks this constraint.
        * @returns value currently stored in given spot
        **/
        bool GetIndex(BaseType index) const {
            return (mInternalRepresentation >> index) & GetOne();
        }

        /**
        * Sets a bit flag in the corresponding spot.
        * Bit manipulation methods from https://stackoverflow.com/a/47990/3735890
        * @param index - Index to set value for. Index starts at 0 and must be less than number of bits in BaseType.
        *                   "Undefined" behavior if input breaks this constraint.
        * @param newValue - New value to set
        **/
        void SetIndex(BaseType index, bool newValue) {
            // Could optimize the if statement out via a bit more complex math, but I like the clarity here atm.
            //      Might as well avoid the pre-optimization until actually willing to performance test either option
            if (newValue) { // Set the flag/spot
                mInternalRepresentation |= GetOne() << index;
            }
            else { // Clear the flag/spot
                mInternalRepresentation &= ~(GetOne() << index);
            }
        }

        BaseType GetAllAsNumber() const {
            return mInternalRepresentation;
        }

        void SetAllAsNumber(BaseType newValue) {
            mInternalRepresentation = newValue;
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&mInternalRepresentation, sizeof(BaseType), CRC::CRC_32(), resultThusFar);
        }

        auto operator<=>(const NumericBitSet& other) const = default;

      private:
        /**
        * Simply gets the value of 1 in the corresponding BaseType.
        *
        * This is intended as a robust workaround for the following SO answer: https://stackoverflow.com/a/47990/3735890
        *   "Use 1ULL if number is wider than unsigned long; promotion of 1UL << n doesn't happen until after
        *       evaluating 1UL << n where it's undefined behaviour to shift by more than the width of a long.
        *       The same applies to all the rest of the examples."
        * @returns value of 1 in correct type
        **/
        static constexpr BaseType GetOne() {
            return 1;
        }
        
        BaseType mInternalRepresentation = 0;
    };
}
