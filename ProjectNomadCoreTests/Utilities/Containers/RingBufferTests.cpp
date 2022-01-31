#include "pch.h"

#include "TestHelpers/TestHelpers.h"
#include "Utilities/Containers/RingBuffer.h"

using namespace ProjectNomad;
namespace RingBufferTests {
    class RingBufferTests : public BaseSimTest {};

    TEST_F(RingBufferTests, getSize_whenEmptyBuffer_returnsFullSize) {
        RingBuffer<int, 3> toTest;

        EXPECT_EQ(3, toTest.getSize());
    }
    
    TEST_F(RingBufferTests, whenSingleElementAdded_successfullyRetrievesAddedElement) {
        RingBuffer<int, 3> toTest;

        toTest.add(123);
        
        EXPECT_EQ(123, toTest.get(0));
    }

    TEST_F(RingBufferTests, whenAddingElementsGreaterThanSize_successfullyRetrievesExistingElements) {
        RingBuffer<int, 3> toTest;

        toTest.add(123);
        toTest.add(456);
        toTest.add(789);
        toTest.add(987);
        toTest.add(654);
        
        EXPECT_EQ(654, toTest.get(0));
        EXPECT_EQ(987, toTest.get(1));
        EXPECT_EQ(789, toTest.get(2));
    }
}