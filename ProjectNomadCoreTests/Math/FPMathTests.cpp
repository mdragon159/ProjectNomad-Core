#include "pch.h"
#include "Math/FixedPoint.h"
#include "Math/FPMath.h"

using namespace ProjectNomad;

namespace FPMathTests {
    TEST(abs, whenPositive_returnsSameValue) {
        fp test = fp{2345.0067f};

        ASSERT_EQ(static_cast<float>(test), static_cast<float>(FPMath::abs(test)));
    }

    TEST(abs, whenNegative_returnsPositiveValue) {
        fp test = fp{-2345.0067f};

        fp expected = fp{2345.0067f};
        ASSERT_EQ(static_cast<float>(expected), static_cast<float>(FPMath::abs(test)));
    }

    TEST(square, returnsExpectedValue) {
        ASSERT_EQ(30.25f, static_cast<float>(FPMath::square(fp{5.5f})));
    }

    //TEST(FPMath, sqrt_whenOne_returnsOne) {
    //  fp test = fp{1};
    //  fp actual = FPMath::sqrt(test);

    //  fp expected = fp{1};
    //  ASSERT_EQ(expected, actual);
    //}
    //
    //TEST(FPMath, sqrt_whenNine_returnsThree) {
    //  fp test = 9;
    //  fp actual = FPMath::sqrt(test);

    //  fp expected = 3;
    //  ASSERT_EQ(expected, actual);
    //}

    TEST(clamp, whenBelowLow_returnsLow) {
        fp test = fp{-5.f};
        fp actual = FPMath::clamp(test, fp{-2}, fp{10});

        fp expected = fp{-2.f};
        ASSERT_EQ(static_cast<float>(expected), static_cast<float>(actual));
    }

    TEST(clamp, whenAboveHigh_returnsLow) {
        fp test = fp{0.5f};
        fp actual = FPMath::clamp(test, fp{-2}, fp{-1});

        fp expected = fp{-1};
        ASSERT_EQ(static_cast<float>(expected), static_cast<float>(actual));
    }

    TEST(clamp, whenValueBetweenRanges_returnsValue) {
        fp test = fp{0.5f};
        fp actual = FPMath::clamp(test, fp{0}, fp{1});

        ASSERT_EQ(static_cast<float>(test), static_cast<float>(actual));
    }

    TEST(fpFmod, whenRemainderExists_returnsRemainder) {
        ASSERT_EQ(1, static_cast<float>(FPMath::fmod(fp{1}, fp{2})));
        ASSERT_EQ(0.5f, static_cast<float>(FPMath::fmod(fp{0.5f}, fp{2})));
    }
    
    TEST(fpFmod, whenNoRemainder_returnsZero) {
        ASSERT_EQ(0, static_cast<float>(FPMath::fmod(fp{2}, fp{1})));
        ASSERT_EQ(0, static_cast<float>(FPMath::fmod(fp{4}, fp{2})));
    }

    TEST(clampAxis, whenEqualToOrAbove360_thenReturnsEquivalentValueBetweenZeroAnd360) {
        ASSERT_EQ(1, static_cast<float>(FPMath::clampAxis(fp{361})));
        ASSERT_EQ(0, static_cast<float>(FPMath::clampAxis(fp{720})));
        ASSERT_EQ(40.5f,  static_cast<float>(FPMath::clampAxis(fp{400.5f})));
        ASSERT_EQ(0, static_cast<float>(FPMath::clampAxis(fp{360})));
    }

    TEST(clampAxis, whenBelowZero_thenReturnsEquivalentAboveZero) {
        ASSERT_EQ(0, static_cast<float>(FPMath::clampAxis(fp{-360})));
        ASSERT_EQ(128.5f, static_cast<float>(FPMath::clampAxis(fp{-231.5f})));
        ASSERT_EQ(1, static_cast<float>(FPMath::clampAxis(fp{-359})));
        ASSERT_EQ(359, static_cast<float>(FPMath::clampAxis(fp{-1})));
    }

    TEST(clampAxis, doesNotClampWhenBetweenExtremes) {
        ASSERT_EQ(0, static_cast<float>(FPMath::clampAxis(fp{0})));
        ASSERT_EQ(14.5f, static_cast<float>(FPMath::clampAxis(fp{14.5f})));
        ASSERT_EQ(180, static_cast<float>(FPMath::clampAxis(fp{180})));
    }

    TEST(normalizeAxis, whenAbove180_thenReturnsEquivalentValueBetweenNegativeAndPositive180) {
        ASSERT_EQ(-179, static_cast<float>(FPMath::normalizeAxis(fp{181})));
        ASSERT_EQ(0, static_cast<float>(FPMath::normalizeAxis(fp{360})));
        ASSERT_EQ(40.5f,  static_cast<float>(FPMath::normalizeAxis(fp{400.5f})));
        ASSERT_EQ(180, static_cast<float>(FPMath::normalizeAxis(fp{180})));
    }

    TEST(normalizeAxis, whenEqualToOrBelowNegative180_thenReturnsEquivalentBetweenNegativeAndPositive180) {
        ASSERT_EQ(0, static_cast<float>(FPMath::normalizeAxis(fp{-360})));
        ASSERT_EQ(128.5f, static_cast<float>(FPMath::normalizeAxis(fp{-231.5f})));
        ASSERT_EQ(1, static_cast<float>(FPMath::normalizeAxis(fp{-359})));
        ASSERT_EQ(180, static_cast<float>(FPMath::normalizeAxis(fp{-180})));
    }

    TEST(normalizeAxis, doesNotClampWhenBetweenExtremes) {
        ASSERT_EQ(0, static_cast<float>(FPMath::normalizeAxis(fp{0})));
        ASSERT_EQ(-14.5f, static_cast<float>(FPMath::normalizeAxis(fp{-14.5f})));
        ASSERT_EQ(180, static_cast<float>(FPMath::normalizeAxis(fp{180})));
        ASSERT_EQ(-1, static_cast<float>(FPMath::normalizeAxis(fp{-1})));
    }

    // <-90, 180>, between is untouched
    // lesser than is clamped to fp{-90}
    // greater is clamped to 180

    TEST(clampAngle, doesNotClampWhenBetweenExtremes) {
        ASSERT_EQ(0, static_cast<float>(FPMath::clampAngle(fp{0}, fp{-90}, fp{180})));
        ASSERT_EQ(120, static_cast<float>(FPMath::clampAngle(fp{120}, fp{-90}, fp{180})));
        ASSERT_EQ(-5.5f, static_cast<float>(FPMath::clampAngle(fp{-5.5f}, fp{-90}, fp{180})));
        ASSERT_EQ(-90, static_cast<float>(FPMath::clampAngle(fp{-90}, fp{-90}, fp{180})));
        ASSERT_EQ(180, static_cast<float>(FPMath::clampAngle(fp{180}, fp{-90}, fp{180})));
    }

    TEST(clampAngle, clampsToExtremes) {
        ASSERT_EQ(-90, static_cast<float>(FPMath::clampAngle(fp{-91}, fp{-90}, fp{180})));
        ASSERT_EQ(180, static_cast<float>(FPMath::clampAngle(fp{-180}, fp{-90}, fp{180})));
        ASSERT_EQ(180, static_cast<float>(FPMath::clampAngle(fp{181}, fp{-90}, fp{180})));
        ASSERT_EQ(-90, static_cast<float>(FPMath::clampAngle(fp{270}, fp{-90}, fp{180})));
    }
    
    TEST(min, whenFirstLessThanSecond_returnsFirst) {
        fp a = fp{1.5f};
        fp b = fp{10.1f};

        ASSERT_NEAR(1.5, (float)FPMath::min(a, b), 0.0001);
    }

    TEST(min, whenSecondLessThanFirst_returnsFirst) {
        fp a = fp{10.1f};
        fp b = fp{1.5f};

        ASSERT_NEAR(1.5, (float)FPMath::min(a, b), 0.0001);
    }

    TEST(max, whenFirstLessThanSecond_returnsSecond) {
        fp a = fp{1.5f};
        fp b = fp{10.1f};

        ASSERT_NEAR(10.1, (float)FPMath::max(a, b), 0.001);
    }

    TEST(max, whenSecondLessThanFirst_returnsFirst) {
        fp a = fp{10.1f};
        fp b = fp{1.5f};

        ASSERT_NEAR(10.1, (float)FPMath::max(a, b), 0.001);
    }

    TEST(degreessToRadians, whenInputIsZero_thenReturnsZero) {
        fp test = FPMath::degreesToRadians(fp{0});

        ASSERT_EQ(0, static_cast<float>(test));
    }

    TEST(degreessToRadians, whenInputIsQuarterRotation_thenReturnsCorrectValue) {
        fp test = FPMath::degreesToRadians(fp{90});

        fp expected = FPMath::getPI() / 2;
        ASSERT_NEAR((float) expected, (float) test, 0.0001);
    }

    TEST(degreessToRadians, whenInputIsHalfRotation_thenReturnsCorrectValue) {
        fp test = FPMath::degreesToRadians(fp{180});

        fp expected = FPMath::getPI();
        ASSERT_NEAR((float) expected, (float) test, 0.0001);
    }

    TEST(degreessToRadians, whenInputIsThreeQuarterRotation_thenReturnsCorrectValue) {
        fp test = FPMath::degreesToRadians(fp{270});

        fp expected = 3 * FPMath::getPI() / 2;
        ASSERT_NEAR((float) expected, (float) test, 0.0001);
    }

    TEST(degreessToRadians, whenInputIsFullRotation_thenReturnsCorrectValue) {
        fp test = FPMath::degreesToRadians(fp{360});

        fp expected = 2 * FPMath::getPI();
        ASSERT_NEAR((float) expected, (float) test, 0.0001);
    }

    TEST(degreessToRadians, whenInputIsNegative_thenReturnsCorrectValue) {
        fp test = FPMath::degreesToRadians(fp{-90});

        fp expected = -1 * FPMath::getPI() / 2;
        ASSERT_NEAR((float) expected, (float) test, 0.0001);
    }

    TEST(radiansToDegrees, whenInputIsZero_thenReturnsZero) {
        fp test = FPMath::radiansToDegrees(fp{0});

        ASSERT_EQ(0, static_cast<float>(test));
    }

    TEST(radiansToDegrees, whenInputIsQuarterRotation_thenReturnsCorrectValue) {
        fp test = FPMath::radiansToDegrees(FPMath::getPI() / 2);

        fp expected = fp{90};
        ASSERT_NEAR((float)expected, (float)test, 0.001);
    }

    TEST(radiansToDegrees, whenInputIsHalfRotation_thenReturnsCorrectValue) {
        fp test = FPMath::radiansToDegrees(FPMath::getPI());

        fp expected = fp{180};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(radiansToDegrees, whenInputIsThreeQuarterRotation_thenReturnsCorrectValue) {
        fp test = FPMath::radiansToDegrees(3 * FPMath::getPI() / 2);

        fp expected = fp{270};
        ASSERT_NEAR((float)expected, (float)test, 0.001);
    }

    TEST(radiansToDegrees, whenInputIsFullRotation_thenReturnsCorrectValue) {
        fp test = FPMath::radiansToDegrees(2 * FPMath::getPI());

        fp expected = fp{360};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(radiansToDegrees, whenInputIsNegative_thenReturnsCorrectValue) {
        fp test = FPMath::radiansToDegrees(-1 * FPMath::getPI());

        fp expected = fp{-180};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(radiansToDegrees, whenNoRotation_returnsExpectedValue) {
        fp test = FPMath::atanR(fp{0}, fp{1});

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(radiansToDegrees, whenQuarterRotation_returnsExpectedValue) {
        fp test = FPMath::atanR(fp{1}, fp{0});

        fp expected = FPMath::getPI() / 2;
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(radiansToDegrees, whenFullRotation_returnsExpectedValue) {
        fp test = FPMath::atanR(fp{-1}, fp{0});

        fp expected = -1 * FPMath::getPI() / 2;
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(radiansToDegrees, when45Degrees_returnsExpectedValue) {
        fp test = FPMath::atanR(fp{1}, fp{1});

        fp expected = FPMath::getPI() / 4;
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(atanD, whenNoRotation_returnsExpectedValue) {
        fp test = FPMath::atanD(fp{0}, fp{1});

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(atanD, whenQuarterRotation_returnsExpectedValue) {
        fp test = FPMath::atanD(fp{1}, fp{0});

        fp expected = fp{90};
        ASSERT_NEAR((float)expected, (float)test, 0.001);
    }

    TEST(atanD, whenFullRotation_returnsExpectedValue) {
        fp test = FPMath::atanD(fp{-1}, fp{0});

        fp expected = fp{-90};
        ASSERT_NEAR((float)expected, (float)test, 0.001);
    }

    TEST(atanD, when45Degrees_returnsExpectedValue) {
        fp test = FPMath::atanD(fp{1}, fp{1});

        fp expected = fp{45};
        ASSERT_NEAR((float)expected, (float)test, 0.001);
    }

    TEST(cos, whenNoRotation_returnsExpectedvalue) {
        fp test = FPMath::cosR(fp{0});

        fp expected = fp{1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cos, whenQuarterRotation_returnsExpectedvalue) {
        fp test = FPMath::cosR(FPMath::getPI() / 2);

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cos, whenHalfRotation_returnsExpectedvalue) {
        fp test = FPMath::cosR(FPMath::getPI());

        fp expected = fp{-1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cos, whenThreeQuarterRotation_returnsExpectedvalue) {
        fp test = FPMath::cosR(3 * FPMath::getPI() / 2);

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cos, when45Degrees_returnsExpectedvalue) {
        fp test = FPMath::cosR(FPMath::getPI() / 4);

        fp expected = FPMath::sqrt(fp{2}) / 2;
        ASSERT_NEAR((float)expected, (float)test, 0.01);
    }

    TEST(cos, whenNegativeValue_returnsExpectedvalue) {
        fp test = FPMath::cosR(-1 * FPMath::getPI() / 2);

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cos, whenMultipleRotations_returnsExpectedvalue) {
        fp test = FPMath::cosR(8 * FPMath::getPI());

        fp expected = fp{1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cosD, whenNoRotation_returnsExpectedvalue) {
        fp test = FPMath::cosD(fp{0});

        fp expected = fp{1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cosD, whenQuarterRotation_returnsExpectedvalue) {
        fp test = FPMath::cosD(fp{90});

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cosD, whenHalfRotation_returnsExpectedvalue) {
        fp test = FPMath::cosD(fp{180});

        fp expected = fp{-1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cosD, whenThreeQuarterRotation_returnsExpectedvalue) {
        fp test = FPMath::cosD(fp{270});

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cosD, when45Degrees_returnsExpectedvalue) {
        fp test = FPMath::cosD(fp{45});

        fp expected = FPMath::sqrt(fp{2}) / 2;
        ASSERT_NEAR((float)expected, (float)test, 0.01);
    }

    TEST(cosD, whenNegativeValue_returnsExpectedvalue) {
        fp test = FPMath::cosD(fp{-90});

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(cosD, whenMultipleRotations_returnsExpectedvalue) {
        fp test = FPMath::cosD(fp{4 * 360});

        fp expected = fp{1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sin, whenNoRotation_returnsExpectedvalue) {
        fp test = FPMath::sinR(fp{0});

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sin, whenQuarterRotation_returnsExpectedvalue) {
        fp test = FPMath::sinR(FPMath::getPI() / 2);

        fp expected = fp{1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sin, whenHalfRotation_returnsExpectedvalue) {
        fp test = FPMath::sinR(FPMath::getPI());

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sin, whenThreeQuarterRotation_returnsExpectedvalue) {
        fp test = FPMath::sinR(3 * FPMath::getPI() / 2);

        fp expected = fp{-1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sin, when45Degrees_returnsExpectedvalue) {
        fp test = FPMath::sinR(FPMath::getPI() / 4);

        fp expected = FPMath::sqrt(fp{2}) / 2;
        ASSERT_NEAR((float)expected, (float)test, 0.01);
    }

    TEST(sin, whenNegativeValue_returnsExpectedvalue) {
        fp test = FPMath::sinR(-1 * FPMath::getPI() / 2);

        fp expected = fp{-1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sin, whenMultipleRotations_returnsExpectedvalue) {
        fp test = FPMath::sinR(8 * FPMath::getPI());

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sinD, whenNoRotation_returnsExpectedvalue) {
        fp test = FPMath::sinD(fp{0});

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sinD, whenQuarterRotation_returnsExpectedvalue) {
        fp test = FPMath::sinD(fp{90});

        fp expected = fp{1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sinD, whenHalfRotation_returnsExpectedvalue) {
        fp test = FPMath::sinD(fp{180});

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sinD, whenThreeQuarterRotation_returnsExpectedvalue) {
        fp test = FPMath::sinD(fp{270});

        fp expected = fp{-1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sinD, when45Degrees_returnsExpectedvalue) {
        fp test = FPMath::sinD(fp{45});

        fp expected = FPMath::sqrt(fp{2}) / 2;
        ASSERT_NEAR((float)expected, (float)test, 0.01);
    }

    TEST(sinD, whenNegativeValue_returnsExpectedvalue) {
        fp test = FPMath::sinD(fp{-90});

        fp expected = fp{-1};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(sinD, whenMultipleRotations_returnsExpectedvalue) {
        fp test = FPMath::sinD(fp{4 * 360});

        fp expected = fp{0};
        ASSERT_NEAR((float)expected, (float)test, 0.0001);
    }

    TEST(swap, successfullySwapsSimpleValues) {
        fp a = fp{-1};
        fp b = fp{2.5f};
        FPMath::swap(a, b);

        ASSERT_EQ(2.5f, static_cast<float>(a));
        ASSERT_EQ(-1, static_cast<float>(b));
    }
}
