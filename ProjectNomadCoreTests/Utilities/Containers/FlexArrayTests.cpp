#include "pch.h"

#include "TestHelpers/TestHelpers.h"
#include "Utilities/Containers/FlexArray.h"

using namespace ProjectNomad;
namespace FlexArrayTests {
    class FlexArrayTests : public BaseSimTest {};

    TEST_F(FlexArrayTests, getMaxSize_whenEmpty_returnsMaxSize) {
        constexpr uint32_t maxSize = 101;
        FlexArray<TestLogger, int, maxSize> toTest(testLogger);
        
        EXPECT_EQ(maxSize, toTest.getMaxSize());
    }

    TEST_F(FlexArrayTests, getSize_whenEmpty_returnsZero) {
        FlexArray<TestLogger, int, 100> toTest(testLogger);
        
        EXPECT_EQ(0, toTest.getSize());
    }

    TEST_F(FlexArrayTests, getSize_whenOneElementAdded_returnsOne) {
        FlexArray<TestLogger, int, 100> toTest(testLogger);
        toTest.add(100);
        
        EXPECT_EQ(1, toTest.getSize());
    }

    TEST_F(FlexArrayTests, getSize_whenOneElementAddedThenRemoved_returnsZero) {
        FlexArray<TestLogger, int, 100> toTest(testLogger);
        toTest.add(100);
        toTest.remove(0);
        
        EXPECT_EQ(0, toTest.getSize());
    }

    TEST_F(FlexArrayTests, get_givenSeveralElementsAdded_thenSuccessfullyReturnsMiddleElement) {
        FlexArray<TestLogger, int, 100> toTest(testLogger);
        toTest.add(123);
        toTest.add(456);
        toTest.add(789);
        
        EXPECT_EQ(456, toTest.get(1));
    }

    TEST_F(FlexArrayTests, get_givenElementRemovedThenAdded_thenSuccessfullyReturnsAddedElement) {
        FlexArray<TestLogger, int, 100> toTest(testLogger);
        toTest.add(123);
        toTest.add(456);
        toTest.add(789);
        toTest.remove(1);
        toTest.add(234);
        
        EXPECT_EQ(234, toTest.get(2));
    }
}