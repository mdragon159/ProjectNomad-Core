#include "pch.h"
#include "Math/FPEulerAngles.h"

using namespace ProjectNomad;

namespace FPEulerAnglesTests {
    TEST(FPEulerAngles, defaultConstructor_returnsExpectedValues) {
        EulerAngles test;

        ASSERT_EQ(0, static_cast<float>(test.roll));
        ASSERT_EQ(0, static_cast<float>(test.pitch));
        ASSERT_EQ(0, static_cast<float>(test.yaw));
    }

    TEST(FPEulerAngles, parameterConstructor_whenGivenBasicValues_returnsValues) {
        EulerAngles test(fp{-20}, fp{1.5}, fp{0});

        ASSERT_EQ(-20, static_cast<float>(test.roll));
        ASSERT_EQ(1.5, static_cast<float>(test.pitch));
        ASSERT_EQ(0, static_cast<float>(test.yaw));
    }

    TEST(FPEulerAngles, zero_returnsZeroAngles) {
        EulerAngles test = EulerAngles::zero();

        ASSERT_EQ(0, static_cast<float>(test.roll));
        ASSERT_EQ(0, static_cast<float>(test.pitch));
        ASSERT_EQ(0, static_cast<float>(test.yaw));
    }

    TEST(FPEulerAngles, inverse_givenEulerWithAllAxes_returnsExpectedAngles) {
        EulerAngles test(fp{1000}, fp{-12.5f}, fp{12345});

        EulerAngles result = -test;

        ASSERT_EQ(-1000, static_cast<float>(result.roll));
        ASSERT_EQ(12.5f, static_cast<float>(result.pitch));
        ASSERT_EQ(-12345, static_cast<float>(result.yaw));
    }

    TEST(toString, returnsExpectedFormatString) {
        EulerAngles test(fp{-20}, fp{1.5}, fp{0});

        std::string expected = "roll: -20.000000 | pitch: 1.500000 | yaw: 0.000000";
        ASSERT_EQ(expected, test.toString());
    }

    TEST(ostreamOutput, outputsCorrectStringForSimpleCase) {
        EulerAngles test(fp{-20}, fp{1.5}, fp{0});

        std::stringstream out;
        out << test;

        std::string expected = "EulerAngles<roll: -20.000000 | pitch: 1.500000 | yaw: 0.000000>";
        ASSERT_EQ(expected, out.str());
    }

    // TOOD: More tests
}
