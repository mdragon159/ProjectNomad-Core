#include "pchNCT.h"

#include "Input/CommandSetList.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace CommandSetListTests {
    class CommandSetListTests : public BaseSimTest {
      protected:
        // Nothing atm
    };

    TEST_F(CommandSetListTests, ValidateRetrievalBehaviorAroundSettingFirstSlot) {
        CommandSetList toTest = {};
        // Be robust and don't unnecessarily hardcode test values
        uint16_t testIndex = 0;
        auto testCommand = static_cast<InputCommand>(testIndex);

        // Starting state validation: Indirectly and directly make sure first bit initializes to 0 
        ASSERT_FALSE(toTest.IsCommandSet(testCommand));
        ASSERT_FALSE(toTest.commandInputs.GetIndex(testIndex));
        ASSERT_EQ(0, toTest.Serialize());

        // Write the value
        toTest.SetCommandValue(testCommand, true);

        // Validate results
        ASSERT_TRUE(toTest.IsCommandSet(testCommand));
        ASSERT_TRUE(toTest.commandInputs.GetIndex(testIndex));
        ASSERT_EQ(1, toTest.Serialize());
    }

    TEST_F(CommandSetListTests, ValidatRetrievalBehaviorAroundSettingLastSlot) {
        CommandSetList toTest = {};
        // Be robust and don't unnecessarily hardcode test values
        uint16_t testIndex = static_cast<int>(InputCommand::ENUM_COUNT) - 1;
        auto testCommand = static_cast<InputCommand>(testIndex);

        // Starting state validation: Indirectly and directly make sure first bit initializes to 0 
        ASSERT_FALSE(toTest.IsCommandSet(testCommand));
        ASSERT_FALSE(toTest.commandInputs.GetIndex(testIndex));
        ASSERT_EQ(0, toTest.Serialize());

        // Write the value
        toTest.SetCommandValue(testCommand, true);

        // Validate results
        ASSERT_TRUE(toTest.IsCommandSet(testCommand));
        ASSERT_TRUE(toTest.commandInputs.GetIndex(testIndex));
        ASSERT_EQ(1 << testIndex, toTest.Serialize());
    }
}
