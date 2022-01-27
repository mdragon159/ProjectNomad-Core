#include "pch.h"

#include "TestHelpers/TestHelpers.h"
#include "Utilities/Containers/InPlaceQueue.h"

using namespace ProjectNomad;
namespace InPlaceQueueTests {
    class InPlaceQueueTests : public BaseSimTest {};

    TEST_F(InPlaceQueueTests, whenEmpty_returnsCorrectSize) {
        constexpr uint32_t maxSize = 101;
        InPlaceQueue<int, maxSize> toTest;
        
        EXPECT_EQ(maxSize, toTest.getMaxSize());
        EXPECT_EQ(0, toTest.getSize());
        EXPECT_TRUE(toTest.isEmpty());
    }

    TEST_F(InPlaceQueueTests, whenElementsAddedAndRemoved_thenHeadTrackingIsAccurate) {
        InPlaceQueue<int, 100> toTest;

        ASSERT_TRUE(toTest.push(123));
        ASSERT_TRUE(toTest.push(456));
        ASSERT_TRUE(toTest.push(789));
        ASSERT_TRUE(toTest.pop());
        
        EXPECT_EQ(2, toTest.getSize());
        EXPECT_EQ(456, toTest.front());
        EXPECT_FALSE(toTest.isEmpty());
    }

    TEST_F(InPlaceQueueTests, clear_whenClearingQueueWithElements_removesAllElements) {
        InPlaceQueue<int, 100> toTest;
        toTest.push(123);
        toTest.push(456);
        toTest.push(789);
        toTest.clear();
        
        EXPECT_EQ(0, toTest.getSize());
    }
}