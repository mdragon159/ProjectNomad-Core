#include "pch.h"

#include "Math/FixedPoint.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace FixedPointTests {
    TEST(toFixedPoint, whenWholeNumber_EqualsSimilarFloat) {
        fp actual {1};

        float expected {1.f};
        ASSERT_EQ(expected, static_cast<float>(actual));
    }

    TEST(toFixedPoint, whenNotWholeNumber_EqualsSimilarFloat) {
        fp actual {2345.5};

        float expected = 2345.5f;
        ASSERT_EQ(expected, static_cast<float>(actual));
    }

    TEST(toFixedPoint, testingVariousNumbers) {
        float expectedA = 500.85f;
        float expectedB = -231.4f;
        float expectedC = 30;
    
        EXPECT_EQ(expectedA, toFloat(fp{expectedA}));
        EXPECT_EQ(expectedB, toFloat(fp{expectedB}));
        EXPECT_EQ(expectedC, toFloat(fp{expectedC}));
    }

    TEST(multiply, simpleFractionMultipliesCorrectly) {
        fp test = fp{3.1};
    
        EXPECT_NEAR(15.5, toFloat(test * 5), 0.001);
    }

    TEST(multiply, doesNotOverflowWhenSquaringTopOfExpectedRange) {
        float base = 45000.12345f;
        fp test = fp{base};
    
        EXPECT_NEAR(base * base, toFloat(test * test), 0.001);
    }

    // TEST(serialization, serializesThenDeserializesSuccessfully) {
    //     float base = 45000.12345f;
    //     fp fpFromBase = fp{base};
    //     
    //     std::stringstream ss;
    //     {
    //         cereal::BinaryOutputArchive outputArchive(ss);
    //         outputArchive(fpFromBase);
    //     }
    //
    //     fp result;
    //     {
    //         cereal::BinaryInputArchive inputArchive(ss);
    //         inputArchive(result);
    //     }
    //
    //     EXPECT_NEAR(base, toFloat(result), 0.01f);
    // }
}
