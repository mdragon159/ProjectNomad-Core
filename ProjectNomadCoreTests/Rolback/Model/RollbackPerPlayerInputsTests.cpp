#include "pchNCT.h"

#include "Rollback/Model/RollbackPerPlayerInputs.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace RollbackPerPlayerInputsTests {
    class RollbackPerPlayerInputsTests : public BaseSimTest {
      protected:
        void SetUp() override {
            // Reset state between runs
            mToTest = {};
        }

        RollbackPerPlayerInputs mToTest = {};
    };
    
    TEST_F(RollbackPerPlayerInputsTests, BasicUsageTestForCompiling) {
        mToTest.SetupForNewSession(GetLoggerSingleton(), {});
        mToTest.AddInput(GetLoggerSingleton(), 0, {});
    }
}
