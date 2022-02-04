#include "pch.h"

#include "TestHelpers/TestHelpers.h"
#include "Utilities/Containers/FlexArray.h"

using namespace ProjectNomad;
namespace FlexArrayTests {
    class FlexArrayTests : public BaseSimTest {};

    TEST_F(FlexArrayTests, getMaxSize_whenEmpty_returnsMaxSize) {
        constexpr uint32_t maxSize = 101;
        FlexArray<int, maxSize> toTest;
        
        EXPECT_EQ(maxSize, toTest.getMaxSize());
    }

    TEST_F(FlexArrayTests, getSize_whenEmpty_returnsZero) {
        FlexArray<int, 100> toTest;
        
        EXPECT_EQ(0, toTest.getSize());
    }

    TEST_F(FlexArrayTests, getSize_whenOneElementAdded_returnsOne) {
        FlexArray<int, 100> toTest;
        toTest.add(100);
        
        EXPECT_EQ(1, toTest.getSize());
    }

    TEST_F(FlexArrayTests, isEmpty_whenEmpty_returnsTrue) {
        FlexArray<int, 3> toTest;
        
        EXPECT_TRUE(toTest.isEmpty());
    }

    TEST_F(FlexArrayTests, isEmpty_whenElementAdded_returnsFalse) {
        FlexArray<int, 3> toTest;
        toTest.add(123);
        
        EXPECT_FALSE(toTest.isEmpty());
    }

    TEST_F(FlexArrayTests, isEmpty_whenElementAddedThenRemoved_returnsTrue) {
        FlexArray<int, 3> toTest;
        toTest.add(123);
        toTest.remove(0);
        
        EXPECT_TRUE(toTest.isEmpty());
    }

    TEST_F(FlexArrayTests, getSize_whenOneElementAddedThenRemoved_returnsZero) {
        FlexArray<int, 100> toTest;
        toTest.add(100);
        toTest.remove(0);
        
        EXPECT_EQ(0, toTest.getSize());
    }

    TEST_F(FlexArrayTests, get_givenSeveralElementsAdded_thenSuccessfullyReturnsMiddleElement) {
        FlexArray<int, 100> toTest;
        toTest.add(123);
        toTest.add(456);
        toTest.add(789);
        
        EXPECT_EQ(456, toTest.get(1));
    }

    TEST_F(FlexArrayTests, get_givenElementRemovedThenAdded_thenSuccessfullyReturnsAddedElement) {
        FlexArray<int, 100> toTest;
        toTest.add(123);
        toTest.add(456);
        toTest.add(789);
        toTest.remove(1);
        toTest.add(234);
        
        EXPECT_EQ(234, toTest.get(2));
    }
    
    TEST_F(FlexArrayTests, remove_whenLoopingOverElementsByIndexAndRemoveElement_thenCanContinueLoopingViaDecrementingIndex) {
        FlexArray<int, 100> toTest;
        toTest.add(123);
        toTest.add(456);
        toTest.add(789);
        toTest.add(234);
        
        bool didRemoval = false;
        bool didSecondPassOnRemovedIndex = false;
        for (uint32_t i = 0; i < toTest.getSize(); i++) {
            switch(i) {
                case 0:
                    ASSERT_EQ(123, toTest.get(0));
                    break;

                case 1:
                    if (!didRemoval) {
                        ASSERT_EQ(456, toTest.get(1));
                        
                        ASSERT_TRUE(toTest.remove(1));
                        didRemoval = true;

                        i--;
                    }
                    else {
                        ASSERT_EQ(234, toTest.get(1));
                        didSecondPassOnRemovedIndex = true;
                    }
                    break;

                case 2:
                    ASSERT_EQ(789, toTest.get(2));
                    break;

                default:
                    FAIL() << "Should not reach default case";
            }
        }

        ASSERT_TRUE(didSecondPassOnRemovedIndex);
    }
}