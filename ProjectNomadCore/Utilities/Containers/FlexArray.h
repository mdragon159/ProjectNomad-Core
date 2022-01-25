#pragma once

#include "Utilities/ILogger.h"

namespace ProjectNomad {
    /// <summary>
    /// Array in place with fixed size but with helpful operations to have dynamic size in place (up to MaxSize).
    /// Intended to be alternative to std::vector for snapshot/memcpy behavior,
    ///     as std::vector's array is effectively a pointer to elsewhere in memory.
    /// NOTE: Order not guaranteed to be in add() call order. Check remove() implementation for more details
    /// </summary>
    template <typename LoggerType, typename ContentType, uint32_t MaxSize>
    class FlexArray {
        static_assert(std::is_base_of_v<ILogger, LoggerType>, "LoggerType must inherit from ILogger");
        static_assert(MaxSize > 0, "MaxSize must be greater than 0");

        LoggerType& logger;
        
        ContentType array[MaxSize];
        uint32_t headIndex = 0; // Points to where next element should be added

    public:
        FlexArray(LoggerType& logger) : logger(logger) {}
        
        static constexpr uint32_t getMaxSize() {
            return MaxSize;
        }

        uint32_t getSize() {
            return headIndex;
        }

        // Returns true if succeeds
        bool add(const ContentType& element) {
            if (headIndex >= MaxSize) {
                logger.logErrorMessage(
                    "FlexArray::add",
                    "Attempted to add element when head already >= MaxSize. Head: " + std::to_string(headIndex)
                            + " and MaxSize: " + std::to_string(MaxSize)
                );
                return false;
            }
            
            array[headIndex] = element;
            headIndex++;
            return true;
        }

        const ContentType& get(uint32_t index) {
            // Check index in bounds
            if (index > MaxSize - 1) {
                logger.logErrorMessage(
                    "FlexArray::get",
                    "Attempted to retrieve outside of MaxSize with index: " + std::to_string(index)
                );
                return array[0];
            }
            if (index >= headIndex) {
                logger.logErrorMessage(
                    "FlexArray::get",
                    "Attempted to retrieve outside of head with head: " + std::to_string(headIndex)
                                + " and index: " + std::to_string(index)
                );
                return array[0];
            }

            // Return desired element
            return array[index];
        }

        // Returns true if succeeds
        bool remove(uint32_t index) {
            // Check index in bounds
            if (index > MaxSize - 1) {
                logger.logErrorMessage(
                    "FlexArray::remove",
                    "Attempted to remove outside of MaxSize with index: " + std::to_string(index)
                );
                return false;
            }
            if (index >= headIndex) {
                logger.logErrorMessage(
                    "FlexArray::remove",
                    "Attempted to remove outside of head with head: " + std::to_string(headIndex)
                                + " and index: " + std::to_string(index)
                );
                return false;
            }
            
            // If not at end, then move element at end into current spot (not retaining order so no need to move all elements around)
            if (index != headIndex - 1) {
                array[index] = array[headIndex - 1];
            }
            headIndex--; // Decrease active size of array by one; element pointed by head is now "invalid"/unused

            return true;
        }
    };
}
