#include "pchNCT.h"

#include "Rollback/Managers/RollbackTimeManager.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace RollbackTimeManagerTests {
    class RollbackTimeManagerTests : public BaseSimTest {
      protected:
        void SetUp() override {
            // Reset state between runs
            mCurTimeInMs = 0;

            std::function<uint64_t()> timeRetriever = std::bind_front(&RollbackTimeManagerTests::GetCurrentTime, this);
            mToTest = RollbackTimeManager(timeRetriever);
        }

        // Function which will retrieve current time for the time manager
        uint64_t GetCurrentTime() const {
            return mCurTimeInMs;
        }

        // Little helper function to help with writing unit tests
        static uint64_t SecondsToMicroSec(const uint64_t seconds) {
            return seconds * 1000 * 1000;
        }

        RollbackTimeManager mToTest = {};
        uint64_t mCurTimeInMs = 0;
    };
    
    TEST_F(RollbackTimeManagerTests, CheckHowManyFramesToProcess_whenCalledForFirstTime_returns1) {
        mToTest.Start();
        FrameType result = mToTest.CheckHowManyFramesToProcess();

        ASSERT_EQ(1, result);
    }
    
    TEST_F(RollbackTimeManagerTests, CheckHowManyFramesToProcess_whenPaused_returns0) {
        mToTest.Start();
        mToTest.Pause();
        FrameType result = mToTest.CheckHowManyFramesToProcess();

        ASSERT_EQ(0, result);
    }

    TEST_F(RollbackTimeManagerTests, CheckHowManyFramesToProcess_whenUnpausedAfterLongTime_returnsMax1) {
        mToTest.Start();
        mToTest.CheckHowManyFramesToProcess(); // Clear initial frame processing special case

        mToTest.Pause();
        mCurTimeInMs = SecondsToMicroSec(10); // Simulate moving 10 seconds into future while paused

        mToTest.Resume();
        FrameType result = mToTest.CheckHowManyFramesToProcess();

        ASSERT_EQ(1, result);
    }

    TEST_F(RollbackTimeManagerTests, CheckHowManyFramesToProcess_whenImmediatelyUnpaused_returns0) {
        mToTest.Start();
        mToTest.CheckHowManyFramesToProcess(); // Clear initial frame processing special case
        
        mToTest.Pause();
        mToTest.Resume();
        FrameType result = mToTest.CheckHowManyFramesToProcess();

        ASSERT_EQ(0, result);
    }

    TEST_F(RollbackTimeManagerTests, CheckHowManyFramesToProcess_whenCalledAfterLongTimeAndNotPaused_returnsBoundedConstant) {
        mToTest.Start();
        mToTest.CheckHowManyFramesToProcess(); // Clear initial frame processing special case
        
        mCurTimeInMs = SecondsToMicroSec(10); // Simulate moving 10 seconds into future
        FrameType result = mToTest.CheckHowManyFramesToProcess();

        FrameType expected = RollbackTimeManager::GetMaxFramesPossibleToProcessAtOnce();
        ASSERT_EQ(expected, result);
    }

    TEST_F(RollbackTimeManagerTests, CheckHowManyFramesToProcess_whenMoveForwardOneFrameAmountOfTime_returns1) {
        mToTest.Start();
        mToTest.CheckHowManyFramesToProcess(); // Clear initial frame processing special case
        
        mCurTimeInMs = static_cast<uint64_t>(FrameRate::TimePerFrameInMicroSec());
        FrameType result = mToTest.CheckHowManyFramesToProcess();

        ASSERT_EQ(1, result);
    }

    TEST_F(RollbackTimeManagerTests, CheckHowManyFramesToProcess_afterPauseAndRestart_successfullyMovesForwardOneFrame) {
        // Get a bit of "noise" into the existing state
        mToTest.Start();
        mToTest.CheckHowManyFramesToProcess(); // Clear initial frame processing special case
        mToTest.Pause();
        mCurTimeInMs = SecondsToMicroSec(10);

        // "Restart" the time manager, which is expected to clear all necessary state
        mToTest.Start();

        // Validate the simple initial frame processing case
        ASSERT_EQ(1, mToTest.CheckHowManyFramesToProcess());

        // Move forward one frame's amount of time
        mCurTimeInMs += static_cast<uint64_t>(FrameRate::TimePerFrameInMicroSec());
        FrameType result = mToTest.CheckHowManyFramesToProcess();
        
        ASSERT_EQ(1, result);
    }
}
