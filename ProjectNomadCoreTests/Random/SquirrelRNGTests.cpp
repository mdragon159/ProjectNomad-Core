#include "pchNCT.h"

#include "Random/SquirrelRNG.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace SquirrelRNGTests {
    class SquirrelRNGTests : public BaseSimTest {
      protected:
        // List of hardcoded known values for deterministic rng checking.
        // Also useful to see how seeds with same positions may differ
        
        static constexpr uint64_t kTestSeed1 = 0;
        static constexpr uint64_t kSeed1FirstRandomVal = 10502344437570651007u; // The u is unnecessary but quiets a "too big for signed literal" warning
        static constexpr uint64_t kSeed1SecondRandomVal = 8378501244249669530;
        static constexpr uint64_t kSeed1ThirdRandomVal = 734454457915330912;

        static constexpr uint64_t kTestSeed2 = 1;
        static constexpr uint64_t kSeed2FirstRandomVal = 12139557187821088049u;
        static constexpr uint64_t kSeed2SecondRandomVal = 10320691743917667832u;
        static constexpr uint64_t kSeed2ThirdRandomVal = 8288123214050453476;

        static constexpr uint64_t kTestSeed3 = 357239;
        static constexpr uint64_t kSeed3FirstRandomVal = 802954267102906604;
        static constexpr uint64_t kSeed3SecondRandomVal = 8926357884514694013;
        static constexpr uint64_t kSeed3ThirdRandomVal = 10775010170493183455u;

        static constexpr uint64_t kTestSeed4 = 198491317;
        static constexpr uint64_t kSeed4FirstRandomVal = 15965933769334680138u;
        static constexpr uint64_t kSeed4SecondRandomVal = 8260785640823660684;
        static constexpr uint64_t kSeed4ThirdRandomVal = 16248715091042061766u;
    };

    TEST_F(SquirrelRNGTests, GetRandom_withFirstTestSeed_returnsExpectedValues) {
        uint64_t resultPos0 = SquirrelRNG::GetRandom(kTestSeed1, 0);
        uint64_t resultPos1 = SquirrelRNG::GetRandom(kTestSeed1, 1);
        uint64_t resultPos2 = SquirrelRNG::GetRandom(kTestSeed1, 2);
        
        EXPECT_EQ(kSeed1FirstRandomVal, resultPos0);
        EXPECT_EQ(kSeed1SecondRandomVal, resultPos1);
        EXPECT_EQ(kSeed1ThirdRandomVal, resultPos2);
    }

    TEST_F(SquirrelRNGTests, GetRandom_withSecondTestSeed_returnsExpectedValues) {
        uint64_t resultPos0 = SquirrelRNG::GetRandom(kTestSeed2, 0);
        uint64_t resultPos1 = SquirrelRNG::GetRandom(kTestSeed2, 1);
        uint64_t resultPos2 = SquirrelRNG::GetRandom(kTestSeed2, 2);

        EXPECT_EQ(kSeed2FirstRandomVal, resultPos0);
        EXPECT_EQ(kSeed2SecondRandomVal, resultPos1);
        EXPECT_EQ(kSeed2ThirdRandomVal, resultPos2);
    }

    TEST_F(SquirrelRNGTests, GetRandom_withThirdTestSeed_returnsExpectedValues) {
        uint64_t resultPos0 = SquirrelRNG::GetRandom(kTestSeed3, 0);
        uint64_t resultPos1 = SquirrelRNG::GetRandom(kTestSeed3, 1);
        uint64_t resultPos2 = SquirrelRNG::GetRandom(kTestSeed3, 2);

        EXPECT_EQ(kSeed3FirstRandomVal, resultPos0);
        EXPECT_EQ(kSeed3SecondRandomVal, resultPos1);
        EXPECT_EQ(kSeed3ThirdRandomVal, resultPos2);
    }

    TEST_F(SquirrelRNGTests, GetRandom_withFourthTestSeed_returnsExpectedValues) {
        uint64_t resultPos0 = SquirrelRNG::GetRandom(kTestSeed4, 0);
        uint64_t resultPos1 = SquirrelRNG::GetRandom(kTestSeed4, 1);
        uint64_t resultPos2 = SquirrelRNG::GetRandom(kTestSeed4, 2);

        EXPECT_EQ(kSeed4FirstRandomVal, resultPos0);
        EXPECT_EQ(kSeed4SecondRandomVal, resultPos1);
        EXPECT_EQ(kSeed4ThirdRandomVal, resultPos2);
    }
}
