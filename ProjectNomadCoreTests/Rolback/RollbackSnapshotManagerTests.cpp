#include "pchNCT.h"

#include "Rollback/RollbackSnapshotManager.h"
#include "TestHelpers/TestHelpers.h"
#include "TestHelpers/TestSnapshot.h"

using namespace ProjectNomad;

namespace RollbackSnapshotManagerTests {
    class RollbackSnapshotManagerTests : public BaseSimTest {
      protected:
        void SetUp() override {
            mToTest.OnSessionStart();
        }

        RollbackSnapshotManager<TestSnapshot> mToTest;
    };
    
    TEST_F(RollbackSnapshotManagerTests, StoreSnapshot_canStoreInitialSnapshot) {
        TestSnapshot toStore;
        toStore.number = 200;
        
        mToTest.StoreSnapshot(0, toStore);

        const TestSnapshot& result = mToTest.GetSnapshot(0);
        EXPECT_EQ(result.number, 200);
    }

    TEST_F(RollbackSnapshotManagerTests, StoreSnapshot_canReplacePriorSnapshot) {
        // Add snapshot for initial frame
        TestSnapshot toStore;
        toStore.number = 1;
        mToTest.StoreSnapshot(0, toStore);
        // Add snapshot for next expected frame
        toStore = {}; // To be safe
        toStore.number = 2;
        mToTest.StoreSnapshot(1, toStore);
        // Replace snapshot for initial frame
        toStore = {};
        toStore.number = 3;
        mToTest.StoreSnapshot(0, toStore);

        const TestSnapshot& initialFrameResult = mToTest.GetSnapshot(0);
        EXPECT_EQ(initialFrameResult.number, 3);
        
        const TestSnapshot& nextFrameResult = mToTest.GetSnapshot(1);
        EXPECT_EQ(nextFrameResult.number, 2);
    }

    TEST_F(RollbackSnapshotManagerTests, StoreSnapshot_whenInsertingOnWrongFrame_givenNoSnapshotsEntered_thenLogsError) {
        TestSnapshot toStore;
        toStore.number = 1;
        mToTest.StoreSnapshot(1, toStore);

        TestHelpers::VerifySingletonLoggingOccured();
    }

    TEST_F(RollbackSnapshotManagerTests, StoreSnapshot_whenInsertingOnWrongFrame_givenSnapshotsEntered_thenLogsError) {
        TestSnapshot toStore;
        toStore.number = 1;
        mToTest.StoreSnapshot(0, toStore);

        TestHelpers::VerifySingleLoggingDidNotOccur();

        toStore = {}; // To be safe
        toStore.number = 2;
        mToTest.StoreSnapshot(2, toStore);

        TestHelpers::VerifySingletonLoggingOccured();
    }
}
