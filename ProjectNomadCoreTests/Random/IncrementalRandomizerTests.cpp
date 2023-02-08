#include "pchNCT.h"

#include "Random/IncrementalRandomizer.h"
#include "Random/SquirrelRNG.h"
#include "Rollback/RollbackSnapshotManager.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace IncrementalRandomizerTests {
    class IncrementalRandomizerTests : public BaseSimTest {
      protected:
        // List of hardcoded known values for deterministic rng checking.
        // Also useful to see how seeds with same positions may differ
        
        static constexpr uint64_t kTestSeed1 = 0;
        static constexpr uint64_t kTest1Min = 0;
        static constexpr uint64_t kTest1Max = 1;
        static constexpr uint64_t kTest1FirstRandomVal = 0; // Note: Given the underlying modulus implementation atm, it's not surprising that there's a statistical preference for 0
        static constexpr uint64_t kTest1SecondRandomVal = 0;
        static constexpr uint64_t kTest1ThirdRandomVal = 0;

        static constexpr uint64_t kTestSeed2 = 1;
        static constexpr uint64_t kTest2Min = 5000;
        static constexpr uint64_t kTest2Max = 357239;
        static constexpr uint64_t kTest2FirstRandomVal = 276391;
        static constexpr uint64_t kTest2SecondRandomVal = 282945;
        static constexpr uint64_t kTest2ThirdRandomVal = 166434;

        static constexpr uint64_t kTestSeed3 = 357239;
        static constexpr uint64_t kTest3Min = 0;
        static constexpr uint64_t kTest3Max = 10;
        static constexpr uint64_t kTest3FirstRandomVal = 4;
        static constexpr uint64_t kTest3SecondRandomVal = 3;
        static constexpr uint64_t kTest3ThirdRandomVal = 5;

        static constexpr uint64_t kTestSeed4 = 198491317;
        static constexpr uint64_t kTest4Min = 0;
        static constexpr uint64_t kTest4Max = 198491317;
        static constexpr uint64_t kTestFirstRandomVal = 69781571;
        static constexpr uint64_t kTest4SecondRandomVal = 172217347;
        static constexpr uint64_t kTest4ThirdRandomVal = 82288079;

        static constexpr uint64_t kTestSeed5 = 198491317;
        static constexpr fp kTest5Min = fp{-10};
        static constexpr fp kTest5Max = fp{10};
        inline static const fp kTest5FirstRandomVal = fp{-6.585808349609375};
        inline static const fp kTest5SecondRandomVal = fp{7.12713623046875};
        inline static const fp kTest5ThirdRandomVal = fp{1.303802490234375};
        inline static const fp kTest5FourthRandomVal = fp{4.6318359375};
        inline static const fp kTest5FifthRandomVal = fp{-9.6868743896484375};
    };

    TEST_F(IncrementalRandomizerTests, GetSeed_returnsSeed) {
        // Yes these should all be separate unit tests but I'm lazy, hate unit tests, and find value extremely little for separation here
        
        IncrementalRandomizer testA;
        EXPECT_EQ(0, testA.GetSeed());

        IncrementalRandomizer testB(kTestSeed3);
        EXPECT_EQ(kTestSeed3, testB.GetSeed());

        testB.SetSeed(kTestSeed4);
        EXPECT_EQ(kTestSeed4, testB.GetSeed());
    }

    TEST_F(IncrementalRandomizerTests, GetPosition_returnsPosition) {
        IncrementalRandomizer testA;
        EXPECT_EQ(0, testA.GetPosition());

        IncrementalRandomizer testB(kTestSeed3);
        EXPECT_EQ(0, testB.GetPosition());

        testB.SetSeed(kTestSeed4);
        testB.SetPosition(kTestSeed2);
        EXPECT_EQ(kTestSeed2, testB.GetPosition());

        testB.GetRandom64(0, 1); // To force internal incrementing
        EXPECT_EQ(kTestSeed2 + 1, testB.GetPosition());
    }

    TEST_F(IncrementalRandomizerTests, GetRandom64_withFirstTestSeed_returnsDeterministicValuesInRange) {
        IncrementalRandomizer toTest(kTestSeed1);
        uint64_t resultPos0 = toTest.GetRandom64(kTest1Min, kTest1Max);
        uint64_t resultPos1 = toTest.GetRandom64(kTest1Min, kTest1Max);
        uint64_t resultPos2 = toTest.GetRandom64(kTest1Min, kTest1Max);
        
        EXPECT_EQ(kTest1FirstRandomVal, resultPos0);
        EXPECT_EQ(kTest1SecondRandomVal, resultPos1);
        EXPECT_EQ(kTest1ThirdRandomVal, resultPos2);
    }

    TEST_F(IncrementalRandomizerTests, GetRandom64_withSecondTestSeed_returnsDeterministicValuesInRange) {
        IncrementalRandomizer toTest(kTestSeed2);
        uint64_t resultPos0 = toTest.GetRandom64(kTest2Min, kTest2Max);
        uint64_t resultPos1 = toTest.GetRandom64(kTest2Min, kTest2Max);
        uint64_t resultPos2 = toTest.GetRandom64(kTest2Min, kTest2Max);
        
        EXPECT_EQ(kTest2FirstRandomVal, resultPos0);
        EXPECT_EQ(kTest2SecondRandomVal, resultPos1);
        EXPECT_EQ(kTest2ThirdRandomVal, resultPos2);
    }

    TEST_F(IncrementalRandomizerTests, GetRandom64_withThirdTestSeed_returnsDeterministicValuesInRange) {
        IncrementalRandomizer toTest(kTestSeed3);
        uint64_t resultPos0 = toTest.GetRandom64(kTest3Min, kTest3Max);
        uint64_t resultPos1 = toTest.GetRandom64(kTest3Min, kTest3Max);
        uint64_t resultPos2 = toTest.GetRandom64(kTest3Min, kTest3Max);
        
        EXPECT_EQ(kTest3FirstRandomVal, resultPos0);
        EXPECT_EQ(kTest3SecondRandomVal, resultPos1);
        EXPECT_EQ(kTest3ThirdRandomVal, resultPos2);
    }

    TEST_F(IncrementalRandomizerTests, GetRandom64_withFourthTestSeed_returnsDeterministicValuesInRange) {
        IncrementalRandomizer toTest(kTestSeed4);
        uint64_t resultPos0 = toTest.GetRandom64(kTest4Min, kTest4Max);
        uint64_t resultPos1 = toTest.GetRandom64(kTest4Min, kTest4Max);
        uint64_t resultPos2 = toTest.GetRandom64(kTest4Min, kTest4Max);
        
        EXPECT_EQ(kTestFirstRandomVal, resultPos0);
        EXPECT_EQ(kTest4SecondRandomVal, resultPos1);
        EXPECT_EQ(kTest4ThirdRandomVal, resultPos2);
    }

    TEST_F(IncrementalRandomizerTests, GetRandomFp_returnsDeterministicValuesInRange) {
        IncrementalRandomizer toTest(kTestSeed5);
        fp resultPos0 = toTest.GetRandomFp(kTest5Min, kTest5Max);
        fp resultPos1 = toTest.GetRandomFp(kTest5Min, kTest5Max);
        fp resultPos2 = toTest.GetRandomFp(kTest5Min, kTest5Max);
        fp resultPos3 = toTest.GetRandomFp(kTest5Min, kTest5Max);
        fp resultPos4 = toTest.GetRandomFp(kTest5Min, kTest5Max);
        
        TestHelpers::expectNear(kTest5FirstRandomVal, resultPos0, fp{0.01f});
        TestHelpers::expectNear(kTest5SecondRandomVal, resultPos1, fp{0.01f});
        TestHelpers::expectNear(kTest5ThirdRandomVal, resultPos2, fp{0.01f});
        TestHelpers::expectNear(kTest5FourthRandomVal, resultPos3, fp{0.01f});
        TestHelpers::expectNear(kTest5FifthRandomVal, resultPos4, fp{0.01f});
    }
}
