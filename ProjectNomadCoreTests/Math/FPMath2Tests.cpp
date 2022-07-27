#include "pchNCT.h"
#include "Math/FPMath2.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace FPMath2Tests {
    TEST(FPMath2, lerpFixedPoint_whenInitialTime_returnsA) {
        fp actual = FPMath2::lerp(fp{0}, fp{1}, fp{0});

        fp expected = fp{0};
        ASSERT_EQ(expected, actual);
    }

    TEST(FPMath2, lerpFixedPoint_whenFinalTime_returnsB) {
        fp actual = FPMath2::lerp(fp{0}, fp{1}, fp{1});

        fp expected = fp{1};
        ASSERT_EQ(expected, actual);
    }

    TEST(FPMath2, lerpFixedPoint_whenBetweenTime_returnsLerpedValue) {
        fp actual = FPMath2::lerp(fp{0}, fp{1}, fp{0.5f});

        fp expected = fp{0.5f};
        ASSERT_EQ(expected, actual);
    }

    TEST(FPMath2, lerpFixedPoint_whenBetweenPosToNegValues_returnsLerpedValue) {
        fp actual = FPMath2::lerp(fp{2}, fp{-10}, fp{0.5f});

        fp expected = fp{-4};
        ASSERT_EQ(expected, actual);
    }

    TEST(FPMath2, lerpVector_whenInitialTime_returnsA) {
        FPVector a = FPVector::zero();
        FPVector b(fp{1}, fp{1}, fp{1});
        FPVector actual = FPMath2::lerp(a, b, fp{0});

        FPVector expected = FPVector::zero();
        ASSERT_EQ(expected, actual);
    }

    TEST(FPMath2, lerpVector_whenFinalTime_returnsB) {
        FPVector a = FPVector::zero();
        FPVector b(fp{1}, fp{1}, fp{1});
        FPVector actual = FPMath2::lerp(a, b, fp{1});

        FPVector expected(fp{1}, fp{1}, fp{1});
        ASSERT_EQ(expected, actual);
    }

    TEST(FPMath2, lerpVector_whenBetweenTime_returnsLerpedValue) {
        FPVector a = FPVector::zero();
        FPVector b(fp{1}, fp{1}, fp{1});
        FPVector actual = FPMath2::lerp(a, b, fp{0.5f});

        FPVector expected(fp{0.5f}, fp{0.5f}, fp{0.5f});
        ASSERT_EQ(expected, actual);
    }

    TEST(FPMath2, lerpVector_whenBetweenPosToNegValues_returnsLerpedValue) {
        FPVector a(fp{2}, fp{5}, fp{-10});
        FPVector b(fp{-10}, fp{1}, fp{1});
        FPVector actual = FPMath2::lerp(a, b, fp{0.5f});

        FPVector expected(fp{-4}, fp{3}, fp{-4.5f});
        ASSERT_EQ(expected, actual);
    }

    TEST(FPMath2, quatToEuler_givenIdentityQuat_returnsZeroEulers) {
        FPQuat input = FPQuat::identity();

        EulerAngles result = FPMath2::quatToEuler(input);

        ASSERT_EQ(EulerAngles::zero(), result);
    }

    TEST(FPMath2, eulerToQuat_givenZeroAngles_returnsNearlyIdentityQuat) {
        EulerAngles input = EulerAngles::zero();

        FPQuat result = FPMath2::eulerToQuat(input);

        FPQuat expected = FPQuat::identity();
        EXPECT_NEAR(toFloat(expected.w), toFloat(result.w), 0.001); // 16 fractional bits are sadly too inaccurate, albeit 20 works well for this
        EXPECT_EQ(expected.v, result.v);
    }

    TEST(FPMath2, eulerToQuat_givenOnlyPitchRotation_returnsExpectedQuat) {
        EulerAngles input;
        input.pitch = fp{-90};

        FPQuat result = FPMath2::eulerToQuat(input);

        ASSERT_NEAR(0.707f, (float)result.w, 0.001);
        ASSERT_NEAR(0, (float)result.v.x, 0.001);
        ASSERT_NEAR(-0.707f, (float)result.v.y, 0.001);
        ASSERT_NEAR(0, (float)result.v.z, 0.001);
    }

    TEST(FPMath2, eulerToQuat_givenRotationOnTwoAxes_returnsExpectedQuat) {
        EulerAngles input;
        input.pitch = fp{180};
        input.roll = fp{90};

        FPQuat result = FPMath2::eulerToQuat(input);

        ASSERT_NEAR(0, (float)result.w, 0.001);
        ASSERT_NEAR(0, (float)result.v.x, 0.001);
        ASSERT_NEAR(0.7071, (float)result.v.y, 0.001);
        ASSERT_NEAR(-0.7071, (float)result.v.z, 0.001);
    }

    TEST(FPMath2, eulerToDirVector_givenSimpleYawAndRoll_returnsExpectedDirection) {
        EulerAngles input;
        input.yaw = fp{90};
        input.roll = fp{45};

        FPVector result = FPMath2::eulerToDirVector(input);

        FPVector expected = FPVector(fp{0}, fp{1}, fp{0});
        TestHelpers::assertNear(expected, result, fp{0.01f});
    }

    TEST(FPMath2, dirVectorToQuat_givenSimpleYawCase_usingQuatResultsInExpectedDirection) {
        FPVector inputDir = FPVector(fp{0}, fp{1}, fp{0});
        FPQuat resultQuat = FPMath2::dirVectorToQuat(inputDir);

        // Note that there are many (infinite?) ways to rotate a direction vector to become a different direction
        // Thus, instead of checking exact values we'll use the quat and confirm the expected result which should be consistent
        FPVector resultRotatedVec = resultQuat * FPVector::forward();
        TestHelpers::assertNear(inputDir, resultRotatedVec, fp{0.01f});
    }

    TEST(FPMath2, dirVectorToQuat_givenOppositeForwardDirection_usingQuatResultsInExpectedDirection) {
        FPVector inputDir = FPVector(fp{-1}, fp{0}, fp{0});
        FPQuat resultQuat = FPMath2::dirVectorToQuat(inputDir);

        // Note that there are many (infinite?) ways to rotate a direction vector to become a different direction
        // Thus, instead of checking exact values we'll use the quat and confirm the expected result which should be consistent
        FPVector resultRotatedVec = resultQuat * FPVector::forward();
        TestHelpers::assertNear(inputDir, resultRotatedVec, fp{0.01f});
    }

    TEST(FPMath2, dirVectorToQuat_givenNearlyOppositeForwardDirection_usingQuatResultsInExpectedDirection) {
        FPVector inputDir = FPVector(fp{-0.999329f}, fp{-0.036804}, fp{0}); // Taken from a game use case that's resulting in problematic rotations
        FPQuat resultQuat = FPMath2::dirVectorToQuat(inputDir);

        // Note that there are many (infinite?) ways to rotate a direction vector to become a different direction
        // Thus, instead of checking exact values we'll use the quat and confirm the expected result which should be consistent
        FPVector resultRotatedVec = resultQuat * FPVector::forward();
        TestHelpers::assertNear(inputDir, resultRotatedVec, fp{0.1f});
    }
    
    TEST(bezierInterp, whenExtremeAlphas_returnsAorBValues) {
        fp a = fp{1};
        fp b = fp{2};
        ASSERT_EQ(1, static_cast<float>(FPMath2::bezierInterp(a, b, fp{0})));
        ASSERT_EQ(2, static_cast<float>(FPMath2::bezierInterp(a, b, fp{1})));
    }

    TEST(bezierInterp, whenGivenMiddleAlpha_returnsMiddleValue) {
        fp a = fp{1};
        fp b = fp{2};
        ASSERT_EQ(1.5f, static_cast<float>(FPMath2::bezierInterp(a, b, fp{0.5f})));
    }

    TEST(interpTo, whenZeroSpeed_returnsTargetValue) {
        FPVector current(fp{-1}, fp{-1}, fp{-1});
        FPVector target(fp{5}, fp{5}, fp{5});

        FPVector expected = target;
        ASSERT_EQ(expected, FPMath2::interpTo(current, target, fp{0}));
    }

    TEST(interpTo, whenDistanceToTargetVerySmall_thenJumpsToTarget) {
        FPVector current(fp{-1}, fp{-1}, fp{-1});
        FPVector target(fp{-1.00000001f}, fp{-1.00000001f}, fp{-1.00000001f});

        FPVector expected = target;
        ASSERT_EQ(expected, FPMath2::interpTo(current, target, fp{0.00000001f}));
    }
    
    TEST(interpTo, whenProvidedDistanceAndTargetAreFarWithLowSpeed_thenReturnsSmallMovementTowardsTarget) {
        FPVector current(fp{-1}, fp{-1}, fp{-1});
        FPVector target(fp{5}, fp{5}, fp{5});

        FPVector expected(fp{-0.5f}, fp{-0.5f}, fp{-0.5f});
        FPVector result = FPMath2::interpTo(current, target, fp{5.f});
        ASSERT_NEAR(static_cast<float>(expected.x), static_cast<float>(result.x), 0.01f);
        ASSERT_NEAR(static_cast<float>(expected.y), static_cast<float>(result.y), 0.01f);
        ASSERT_NEAR(static_cast<float>(expected.z), static_cast<float>(result.z), 0.01f);
    }

    // TODO: Euler tests for more complex angles (esp w/ all 3 angles set), or at least for yaw explicitly. 
    // Good Euler-Quat test examples:
    // https://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/steps/index.htm
}
