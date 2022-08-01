#include "pchNCT.h"

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

        toTest.Add(123);
        
        EXPECT_EQ(123, toTest.Get(0));
    }

    TEST_F(RingBufferTests, whenAddingElementsGreaterThanSize_successfullyRetrievesExistingElements) {
        RingBuffer<int, 3> toTest;

        toTest.Add(123);
        toTest.Add(456);
        toTest.Add(789);
        toTest.Add(987);
        toTest.Add(654);
        
        EXPECT_EQ(654, toTest.Get(0));
        EXPECT_EQ(987, toTest.Get(1));
        EXPECT_EQ(789, toTest.Get(2));
    }

    TEST_F(RingBufferTests, swapInsert_swapsAndInsertsAsExpected) {
        RingBuffer<int, 3> toTest;

        toTest.Add(12);
        toTest.Add(45);
        toTest.Add(78);
        toTest.Add(90);

        int val = 1234;
        toTest.SwapInsert(val);
        
        EXPECT_EQ(1234, toTest.Get(0));
        EXPECT_EQ(90, toTest.Get(1));
        EXPECT_EQ(78, toTest.Get(2));

        // Perhaps shouldn't be testing this as no intention of swapped-out value being useful,
        //  but might as well verify behavior atm
        EXPECT_EQ(45, val);
    }

    TEST_F(RingBufferTests, SwapReplace_worksAsExpected) {
        RingBuffer<int, 4> toTest;

        toTest.Add(12);
        toTest.Add(45);
        toTest.Add(78);
        toTest.Add(90);
        toTest.Add(999);

        int replaceVal = 1234;
        toTest.SwapReplace(0, replaceVal);
        replaceVal = 2345;
        toTest.SwapReplace(3, replaceVal);
        replaceVal = 3456;
        toTest.SwapReplace(1,  replaceVal);
        
        EXPECT_EQ(1234, toTest.Get(0));
        EXPECT_EQ(3456, toTest.Get(1));
        EXPECT_EQ(78, toTest.Get(2));
        EXPECT_EQ(2345, toTest.Get(3));
    }
}