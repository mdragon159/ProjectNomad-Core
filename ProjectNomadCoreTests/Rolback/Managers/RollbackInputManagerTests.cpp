#include "pchNCT.h"

#include "Rollback/Managers/RollbackInputManager.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace RollbackInputManagerTests {
    class RollbackInputManagerTests : public BaseSimTest {
      protected:
        void SetUp() override {
            // Reset state between runs
            mToTest = {};
        }

        RollbackInputManager mToTest = {};
    };
    
    TEST_F(RollbackInputManagerTests, BasicUsageTestForCompiling) {
        mToTest.SetupForNewSession(GetLoggerSingleton(), {});
        mToTest.SetInputForPlayer(GetLoggerSingleton(), 0, PlayerSpot::Player1, {});
    }
}
