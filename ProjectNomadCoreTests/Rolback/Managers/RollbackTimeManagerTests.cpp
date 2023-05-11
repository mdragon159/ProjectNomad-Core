#include "pchNCT.h"

#include "Rollback/Managers/RollbackInputManager.h"
#include "Rollback/Managers/RollbackTimeManager.h"
#include "Rollback/Model/RollbackPerPlayerInputs.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace RollbackTimeManagerTests {
    class RollbackTimeManagerTests : public BaseSimTest {
      protected:
        void SetUp() override {
            // Reset state between runs
            mToTest = {};
        }

        RollbackTimeManager mToTest = {};
    };
    
    TEST_F(RollbackTimeManagerTests, BasicUsageTestForCompiling) {
        mToTest.Start();
    }
}
