#include "pchNCT.h"

#include "Rollback/RollbackManager.h"
#include "Rollback/Managers/RollbackSnapshotManager.h"
#include "TestHelpers/TestHelpers.h"
#include "TestHelpers/TestSnapshot.h"
#include "TestHelpers/Rollback/RollbackTestUser.h"

using namespace ProjectNomad;

namespace RollbackManagerTests {
    class RollbackManagerTests : public BaseSimTest {
      protected:
        void SetUp() override {
            // Reset any possibly changed state between runs
            mRollbackTestUser = {};
        }

        RollbackTestUser mRollbackTestUser = {};
        RollbackManager<TestSnapshot> mToTest = RollbackManager(mRollbackTestUser);
    };
    
    TEST_F(RollbackManagerTests, BasicUsageTestForCompiling) {
        // Just call any method to assure stuff do be used
        mToTest.StartRollbackSession({});
        mToTest.OnTick();
    }
}
