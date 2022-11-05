#include "pchNCT.h"

#include "Input/PlayerInput.h"
#include "TestHelpers/TestHelpers.h"
#include "Utilities/Containers/FlexArray.h"

using namespace ProjectNomad;
namespace FlexArrayTests {
    class FlexArrayTests : public BaseSimTest {};

    TEST_F(FlexArrayTests, GetMaxSize_whenEmpty_returnsMaxSize) {
        constexpr uint32_t maxSize = 101;
        FlexArray<int, maxSize> toTest;
        
        EXPECT_EQ(maxSize, toTest.GetMaxSize());
    }

    TEST_F(FlexArrayTests, GetSize_whenEmpty_returnsZero) {
        FlexArray<int, 100> toTest;
        
        EXPECT_EQ(0, toTest.GetSize());
    }

    TEST_F(FlexArrayTests, GetSize_whenOneElementAdded_returnsOne) {
        FlexArray<int, 100> toTest;
        toTest.Add(100);
        
        EXPECT_EQ(1, toTest.GetSize());
    }

    TEST_F(FlexArrayTests, IsEmpty_whenEmpty_returnsTrue) {
        FlexArray<int, 3> toTest;
        
        EXPECT_TRUE(toTest.IsEmpty());
    }

    TEST_F(FlexArrayTests, IsEmpty_whenElementAdded_returnsFalse) {
        FlexArray<int, 3> toTest;
        toTest.Add(123);
        
        EXPECT_FALSE(toTest.IsEmpty());
    }

    TEST_F(FlexArrayTests, IsEmpty_whenElementAddedThenRemoved_returnsTrue) {
        FlexArray<int, 3> toTest;
        toTest.Add(123);
        toTest.Remove(0);
        
        EXPECT_TRUE(toTest.IsEmpty());
    }

    TEST_F(FlexArrayTests, IsFull_whenEmpty_returnsFalse) {
        FlexArray<int, 3> toTest;
        
        EXPECT_FALSE(toTest.IsFull());
    }

    TEST_F(FlexArrayTests, IsFull_whenSingleElementAdded_returnsFalse) {
        FlexArray<int, 3> toTest;
        toTest.Add(123);
        
        EXPECT_FALSE(toTest.IsFull());
    }

    TEST_F(FlexArrayTests, IsFull_whenMaxCapacityElementsAdded_returnsTrue) {
        FlexArray<int, 3> toTest;
        toTest.Add(123);
        toTest.Add(123);
        toTest.Add(123);
        
        EXPECT_TRUE(toTest.IsFull());
    }

    TEST_F(FlexArrayTests, GetSize_whenOneElementAddedThenRemoved_returnsZero) {
        FlexArray<int, 100> toTest;
        toTest.Add(100);
        toTest.Remove(0);
        
        EXPECT_EQ(0, toTest.GetSize());
    }

    TEST_F(FlexArrayTests, Get_givenSeveralElementsAdded_thenSuccessfullyReturnsMiddleElement) {
        FlexArray<int, 100> toTest;
        toTest.Add(123);
        toTest.Add(456);
        toTest.Add(789);
        
        EXPECT_EQ(456, toTest.Get(1));
    }

    TEST_F(FlexArrayTests, Get_givenElementRemovedThenAdded_thenSuccessfullyReturnsAddedElement) {
        FlexArray<int, 100> toTest;
        toTest.Add(123);
        toTest.Add(456);
        toTest.Add(789);
        toTest.Remove(1);
        toTest.Add(234);
        
        EXPECT_EQ(234, toTest.Get(2));
    }

    TEST_F(FlexArrayTests, Contains_whenEmpty_returnsFalse) {
        FlexArray<int, 100> toTest;
        EXPECT_FALSE(toTest.Contains(0));
    }

    TEST_F(FlexArrayTests, Contains_whenElementsAddedAndRemoved_whenCalledWithContainedElement_returnsTrue) {
        FlexArray<int, 100> toTest;
        toTest.Add(123);
        toTest.Add(456);
        toTest.Add(789);
        toTest.Remove(1);
        toTest.Add(234);
        
        EXPECT_TRUE(toTest.Contains(789));
    }

    TEST_F(FlexArrayTests, Contains_whenElementsAddedAndRemoved_whenCalledWithRemovedElement_returnsFalse) {
        FlexArray<int, 100> toTest;
        toTest.Add(123);
        toTest.Add(456);
        toTest.Add(789);
        toTest.Remove(1);
        toTest.Add(234);
        
        EXPECT_FALSE(toTest.Contains(456));
    }
    
    TEST_F(FlexArrayTests, Remove_whenLoopingOverElementsByIndexAndRemoveElement_thenCanContinueLoopingViaDecrementingIndex) {
        FlexArray<int, 100> toTest;
        toTest.Add(123);
        toTest.Add(456);
        toTest.Add(789);
        toTest.Add(234);
        
        bool didRemoval = false;
        bool didSecondPassOnRemovedIndex = false;
        for (uint32_t i = 0; i < toTest.GetSize(); i++) {
            switch(i) {
                case 0:
                    ASSERT_EQ(123, toTest.Get(0));
                    break;

                case 1:
                    if (!didRemoval) {
                        ASSERT_EQ(456, toTest.Get(1));
                        
                        ASSERT_TRUE(toTest.Remove(1));
                        didRemoval = true;

                        i--;
                    }
                    else {
                        ASSERT_EQ(234, toTest.Get(1));
                        didSecondPassOnRemovedIndex = true;
                    }
                    break;

                case 2:
                    ASSERT_EQ(789, toTest.Get(2));
                    break;

                default:
                    FAIL() << "Should not reach default case";
            }
        }

        ASSERT_TRUE(didSecondPassOnRemovedIndex);
    }

    TEST_F(FlexArrayTests, AddAll_whenOneListEmpty_thenAddsNothing) {
        FlexArray<int, 1> firstTest;
        firstTest.Add(123);

        FlexArray<int, 1> secondTest;

        bool didAdd = firstTest.AddAll(secondTest);
        EXPECT_TRUE(didAdd);
    }

    TEST_F(FlexArrayTests, AddAll_whenAddingMultipleElements_addsExpectedElements) {
        FlexArray<int, 100> firstTest;
        firstTest.Add(123);
        firstTest.Add(456);
        firstTest.Add(789);
        firstTest.Remove(1);
        firstTest.Add(234);

        FlexArray<int, 100> secondTest;
        secondTest.Add(111);
        secondTest.Add(222);
        secondTest.Add(333);
        secondTest.Remove(1);
        secondTest.Add(444);

        bool didAdd = firstTest.AddAll(secondTest);
        EXPECT_TRUE(didAdd);

        EXPECT_EQ(6, firstTest.GetSize());
        EXPECT_EQ(123, firstTest.Get(0));
        EXPECT_EQ(789, firstTest.Get(1));
        EXPECT_EQ(234, firstTest.Get(2));
        EXPECT_EQ(111, firstTest.Get(3));
        EXPECT_EQ(333, firstTest.Get(4));
        EXPECT_EQ(444, firstTest.Get(5));
    }
    
    TEST_F(FlexArrayTests, CalculateCRC32_whenSameValues_givenIntType_thenChecksumAreEquivalent) {
        FlexArray<int, 100> firstTest;
        firstTest.Add(123);
        firstTest.Add(456);
        firstTest.Add(789);
        firstTest.Remove(1);
        firstTest.Add(234);

        FlexArray<int, 100> secondTest;
        secondTest.Add(123);
        secondTest.Add(456);
        secondTest.Add(789);
        secondTest.Remove(1);
        secondTest.Add(234);

        uint32_t firstChecksum = 0;
        firstTest.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondTest.CalculateCRC32(secondChecksum);
        
        EXPECT_EQ(firstChecksum, secondChecksum);
    }

    TEST_F(FlexArrayTests, CalculateCRC32_whenDifferentValues_givenIntType_thenChecksumAreDifferent) {
        FlexArray<int, 100> firstTest;
        firstTest.Add(123);
        firstTest.Add(456);
        firstTest.Add(789);
        firstTest.Remove(0);
        firstTest.Add(234);

        FlexArray<int, 100> secondTest;
        secondTest.Add(123);
        secondTest.Add(456);
        secondTest.Add(789);
        secondTest.Remove(1);
        secondTest.Add(234);

        uint32_t firstChecksum = 0;
        firstTest.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondTest.CalculateCRC32(secondChecksum);
        
        EXPECT_NE(firstChecksum, secondChecksum);
    }

    TEST_F(FlexArrayTests, CalculateCRC32_whenSameValues_givenComplexType_thenChecksumAreEquivalent) {
        FlexArray<PlayerInput, 100> firstTest;
        PlayerInput inputA = {};
        inputA.moveForward = fp{0.5f};
        inputA.commandInputs.SetCommandValue(InputCommand::Jump, true);
        inputA.commandInputs.SetCommandValue(InputCommand::AttackHeavy, true);
        firstTest.Add(inputA);
        
        FlexArray<PlayerInput, 100> secondTest;
        PlayerInput inputB = {};
        inputB.moveForward = fp{0.5f};
        inputB.commandInputs.SetCommandValue(InputCommand::Jump, true);
        inputB.commandInputs.SetCommandValue(InputCommand::AttackHeavy, true);
        secondTest.Add(inputB);

        uint32_t firstChecksum = 0;
        firstTest.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondTest.CalculateCRC32(secondChecksum);
        
        EXPECT_EQ(firstChecksum, secondChecksum);
    }

    TEST_F(FlexArrayTests, CalculateCRC32_whenDifferentValues_givenComplexType_thenChecksumAreDifferent) {
        FlexArray<PlayerInput, 100> firstTest;
        PlayerInput inputA = {};
        inputA.moveForward = fp{0.5f};
        inputA.commandInputs.SetCommandValue(InputCommand::Jump, true);
        inputA.commandInputs.SetCommandValue(InputCommand::AttackHeavy, true);
        firstTest.Add(inputA);

        FlexArray<PlayerInput, 100> secondTest;
        PlayerInput inputB = {};
        inputB.moveForward = fp{0.25f};
        inputB.commandInputs.SetCommandValue(InputCommand::AttackLight, true);
        secondTest.Add(inputB);

        uint32_t firstChecksum = 0;
        firstTest.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondTest.CalculateCRC32(secondChecksum);
        
        EXPECT_NE(firstChecksum, secondChecksum);
    }
    
}