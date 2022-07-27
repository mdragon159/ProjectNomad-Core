#include "pchNCT.h"
#include <sstream>

#include "Math/FPQuat.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace FPQuatTests {
    TEST(FPQuat, defaultConstructor_createsQuatSuccessfully) {
        FPQuat test;

        ASSERT_EQ(0, static_cast<float>(test.w));
        TestHelpers::expectNear(FPVector::zero(), test.v, fp{0});
    }

    TEST(FPQuat, parameterConstructor_whenGivenValuesAllOverThePlace_returnsQuatWithValues) {
        FPVector expectedV(fp{-100}, fp{1.5f}, fp{2});
        FPQuat test(fp{-1000}, expectedV);

        ASSERT_EQ(-1000, static_cast<float>(test.w));
        TestHelpers::expectNear(expectedV, test.v, fp{0});
    }

    TEST(FPQuat, fromRadians_whenRotated90DegreesAroundX_givesExpectedQuat) {
        FPQuat test = FPQuat::fromRadians({fp{1}, fp{0}, fp{0}}, FPMath::getPI() / 2);;

        fp sqrtTwoOverTwo = FPMath::sqrt(fp{2}) / 2;
        ASSERT_NEAR(static_cast<float>(sqrtTwoOverTwo), (float) test.w, 0.001f);
        ASSERT_NEAR(static_cast<float>(sqrtTwoOverTwo), (float) test.v.x, 0.001f);
        ASSERT_NEAR(0, (float) test.v.y, 0.001f);
        ASSERT_NEAR(0, (float) test.v.z, 0.001f);
    }

    TEST(FPQuat, fromDegrees_whenRotated90DegreesAroundX_givesExpectedQuat) {
        FPQuat test = FPQuat::fromDegrees({fp{1}, fp{0}, fp{0}}, fp{90});

        fp sqrtTwoOverTwo = FPMath::sqrt(fp{2}) / 2;
        ASSERT_NEAR(static_cast<float>(sqrtTwoOverTwo), (float) test.w, 0.001f);
        ASSERT_NEAR(static_cast<float>(sqrtTwoOverTwo), (float) test.v.x, 0.001f);
        ASSERT_NEAR(0, (float) test.v.y, 0.001f);
        ASSERT_NEAR(0, (float) test.v.z, 0.001f);
    }

    TEST(FPQuat, identity_createsExpectedQuat) {
        FPQuat test = FPQuat::identity();

        FPQuat expected = FPQuat(fp{1}, FPVector::zero());
        ASSERT_EQ(expected, test);
    }

    TEST(FPQuat, inverted_whenSimpleRotationQuat_returnsConjugateQuat) {
        FPVector expectedV(fp{-100}, fp{1.5f}, fp{2});
        FPQuat test(fp{0.45f}, expectedV);
        FPQuat actual = test.inverted();

        EXPECT_NEAR(0.45f, static_cast<float>(test.w), 0.1);
        ASSERT_EQ(-expectedV.x, actual.v.x);
        ASSERT_EQ(-expectedV.y, actual.v.y);
        ASSERT_EQ(-expectedV.z, actual.v.z);
    }

    TEST(FPQuat, multiplyQuats_whenSimpleRotations_returnsExpectedQuat) {
        FPQuat a = FPQuat::fromDegrees({fp{0}, fp{1}, fp{0}}, fp{90});
        FPQuat b = FPQuat::fromDegrees({fp{0}, fp{0}, fp{-1}}, fp{90});
        FPQuat rotatedQuat = a * b;

        ASSERT_NEAR(0.5f, (float) rotatedQuat.w, 0.001f);
        ASSERT_NEAR(-0.5, (float) rotatedQuat.v.x, 0.001f);
        ASSERT_NEAR(0.5f, (float) rotatedQuat.v.y, 0.001f);
        ASSERT_NEAR(-0.5, (float) rotatedQuat.v.z, 0.001f);
    }

    TEST(FPQuat, multiplyVector_whenSimpleRotatedPoint_returnsExpectedPoint) {
        FPVector point(fp{1}, fp{0}, fp{0});
        FPQuat rotation = FPQuat::fromDegrees({fp{0}, fp{0}, fp{-1}}, fp{90});
        FPVector rotatedPoint = rotation * point;

        FPVector expected(fp{0}, fp{-1}, fp{0});
        TestHelpers::expectNear(expected, rotatedPoint, fp{0.001f});
    }

    TEST(FPQuat, multiplyVector_whenRotatingForwardVector_returnsIntendedRotationAsDirVector) {
        FPVector forward = FPVector::forward();
        FPQuat rotation = FPQuat::fromDegrees({fp{0}, fp{0}, fp{-1}}, fp{90});
        FPVector rotatedDirVector = rotation * forward;

        FPVector expected(fp{0}, fp{-1}, fp{0});
        TestHelpers::expectNear(expected, rotatedDirVector, fp{0.001f});
    }

    TEST(FPQuat, equals_whenVerySimpleEquivalentQuats_returnsTrue) {
        FPQuat a = FPQuat(fp{0.5f}, {fp{0}, fp{2}, fp{-1}});
        FPQuat b = FPQuat(fp{0.5f}, {fp{0}, fp{2}, fp{-1}});

        ASSERT_EQ(a, b);
    }

    TEST(toString, outputsCorrectStringForSimpleFPQuat) {
        FPQuat identity = FPQuat::identity();

        ASSERT_EQ("1.000000, x: 0.000000 | y: 0.000000 | z: 0.000000", identity.toString());
    }

    TEST(ostreamOutput, outputsCorrectStringForSimpleFPQuat) {
        FPQuat identity = FPQuat::identity();

        std::stringstream out;
        out << identity;

        ASSERT_EQ("FPQuat<1, FPVector<0, 0, 0>>", out.str());
    }


    // TODO: More tests for...
    // fromDegrees, fromRadians, quat mult, vector mult
    // ==, !=, other side multiply, identity quat
}
