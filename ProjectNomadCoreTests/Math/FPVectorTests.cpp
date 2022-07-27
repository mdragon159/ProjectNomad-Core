#include "pchNCT.h"
#include <sstream>

#include "Math/FPVector.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace FPVectorTests {
    TEST(FPVector, getLength_whenSingleAxisValue_returnsValue) {
        FPVector test(fp{1}, fp{0}, fp{0});

        ASSERT_EQ(1, static_cast<float>(test.getLength()));
    }

    TEST(FPVector, getLength_whenVectorHasXYZValues_calculatesLengthCorrectly) {
        FPVector test(fp{3}, fp{5}, fp{7});

        // TODO: Round to 9.11 so don't need to fix every time fp rep changes
        //ASSERT_EQ(9.11043, test.getLength());
        ASSERT_NEAR(9.11f, static_cast<float>(test.getLength()), 0.001f);
    }

    TEST(FPVector, invert_whenSimpleVector_returnsInvertedVector) {
        FPVector test(fp{1}, fp{1}, fp{1});
        FPVector actual = -test;

        FPVector expected(fp{-1}, fp{-1}, fp{-1});
        ASSERT_EQ(expected, actual);
    }

    TEST(FPVector, add_whenTwoSimpleVectors_returnsAddedVector) {
        FPVector A(fp{1}, fp{2}, fp{3});
        FPVector B(fp{4}, fp{-2}, fp{1.5f});
        FPVector result = A + B;

        TestHelpers::expectNear(FPVector(fp{5}, fp{0}, fp{4.5f}), result, fp{0.0001f});
    }

    TEST(FPVector, add_whenVectorAddedToSelf_returnsAddedVector) {
        FPVector test(fp{1}, fp{0}, fp{0});
        FPVector result = test + test;

        TestHelpers::expectNear(FPVector(fp{2}, fp{0}, fp{0}), result, fp{0.0001f});
        ASSERT_NEAR(2, static_cast<float>(result.getLength()), 0.0001);
    }

    TEST(FPVector, addAssignment_whenTwoSimpleVectors_addsSecondVectorToFirst) {
        FPVector A(fp{1}, fp{2}, fp{3});
        FPVector B(fp{4}, fp{-2}, fp{1.5});

        A += B;

        FPVector expected(fp{5}, fp{0}, fp{4.5f});
        TestHelpers::expectNear(expected, A, fp{0.0001f});
    }

    TEST(FPVector, subtract_whenTwoSimpleVectors_returnsSubtractedVector) {
        FPVector A(fp{1}, fp{2}, fp{3});
        FPVector B(fp{4}, fp{-2}, fp{1.5f});
        FPVector result = A - B;

        FPVector expected(fp{-3}, fp{4}, fp{1.5f});
        TestHelpers::expectNear(expected, result, fp{0.0001f});
    }

    TEST(FPVector, subtract_whenVectorSubtractedFromSelf_returnsZeroVector) {
        FPVector test(fp{1}, fp{2}, fp{-3});
        FPVector result = test - test;

        ASSERT_NEAR(0, (float)result.x, 0.0001);
        ASSERT_NEAR(0, (float)result.y, 0.0001);
        ASSERT_NEAR(0, (float)result.z, 0.0001);
        ASSERT_NEAR(0, (float)result.getLength(), 0.0001);
    }

    TEST(FPVector, subtractAssignment_whenTwoSimpleVectors_subtractsSecondFromFirst) {
        FPVector A(fp{1}, fp{2}, fp{3});
        FPVector B(fp{4}, fp{-2}, fp{1.5});

        A -= B;

        ASSERT_NEAR(-3, (float)A.x, 0.0001);
        ASSERT_NEAR(4, (float)A.y, 0.0001);
        ASSERT_NEAR(1.5, (float)A.z, 0.0001);
    }

    TEST(FPVector, multiplyScalar_whenMultiplyAllAxes_returnsVectorWithAllAxesModified) {
        FPVector test(fp{1}, fp{2.5f}, fp{3.5f});
        FPVector result = test * fp{5};

        FPVector expected(fp{5}, fp{12.5f}, fp{17.5f});
        ASSERT_EQ(expected, result);
    }

    TEST(FPVector, divideScaler_whenDivideAllAxes_returnsVectorWithAllAxesModified) {
        FPVector test(fp{1}, fp{2}, fp{3});
        FPVector result = test / fp{2};

        FPVector expected(fp{0.5f}, fp{1}, fp{1.5f});
        ASSERT_EQ(expected, result);
    }

    TEST(FPVector, normalize_whenNotNormal_appliesNormalizationToSelf) {
        FPVector test(fp{3}, fp{4}, fp{0});
        
        test.normalize();

        FPVector expected(fp{0.6}, fp{0.8}, fp{0});
        TestHelpers::expectNear(expected, test, fp{0.001});
    }

    TEST(FPVector, normalized_whenAlreadyNormal_returnsSameVector) {
        FPVector test(fp{1}, fp{0}, fp{0});

        ASSERT_EQ(test, test.normalized());
    }

    TEST(FPVector, normalized_whenNotNormal_returnsNormalizedVector) {
        FPVector test(fp{3}, fp{4}, fp{0});

        FPVector normalized = test.normalized();

        FPVector expected(fp{0.6}, fp{0.8}, fp{0});
        TestHelpers::expectNear(expected, normalized, fp{0.001});
    }

    TEST(FPVector, normalized_whenLargeValue_returnsNormalizedVector) {
        FPVector test(fp{500.85f}, fp{-231.4f}, fp{30});
        // test = FPVector::zero();

        FPVector normalized = test.normalized();

        FPVector expected(fp{0.91}, fp{-0.42}, fp{0.05});
        TestHelpers::expectNear(expected, normalized, fp{0.01});
    }

    TEST(FPVector, dotProduct_whenTwoPerpendicularVectors_returnsZero) {
        FPVector A(fp{1}, fp{0}, fp{0});
        FPVector B(fp{0}, fp{1}, fp{0});
        fp result = A.dot(B);

        ASSERT_NEAR(0, (float)result, 0.0001);
    }

    TEST(FPVector, dotProduct_whenSameVector_returnsLengthMultipliedByItself) {
        FPVector test(fp{1}, fp{2}, fp{3});
        fp result = test.dot(test);

        fp expected = test.getLength() * test.getLength();
        ASSERT_NEAR((float)expected, (float)result, 0.0001);
    }

    TEST(FPVector, dotProduct_whenParallelVectors_returnsExpectedResult) {
        FPVector A(fp{1}, fp{0}, fp{0});
        FPVector B(fp{2}, fp{0}, fp{0});
        fp result = A.dot(B);

        ASSERT_NEAR(2, (float)result, 0.0001);
    }

    TEST(FPVector, dotProduct_whenTwoParallelVectors_whenDotProductBothWays_returnsSameResult) {
        FPVector A(fp{1}, fp{0}, fp{0});
        FPVector B(fp{2}, fp{0}, fp{0});
        fp resultA = A.dot(B);
        fp resultB = B.dot(A);

        ASSERT_NEAR((float) resultB, (float)resultA, 0.0001);
    }

    TEST(FPVector, crossProduct_whenTwoAxes_getThirdAxis) {
        FPVector A(fp{1}, fp{0}, fp{0});
        FPVector B(fp{0}, fp{1}, fp{0});

        FPVector expected(fp{0}, fp{0}, fp{1});
        ASSERT_EQ(expected, A.cross(B));
    }

    TEST(FPVector, crossProduct_whenTwoAxesButReversed_getThirdAxisReversed) {
        FPVector A(fp{1}, fp{0}, fp{0});
        FPVector B(fp{0}, fp{1}, fp{0});

        FPVector expected(fp{0}, fp{0}, fp{-1});
        ASSERT_EQ(expected, B.cross(A));
    }

    TEST(FPVector, crossProduct_whenTwoAxesButOneNegative_getThirdAxisReversed) {
        FPVector A(fp{-1}, fp{0}, fp{0});
        FPVector B(fp{0}, fp{1}, fp{0});

        FPVector expected(fp{0}, fp{0}, fp{-1});
        ASSERT_EQ(expected, A.cross(B));
    }

    TEST(FPVector, crossProduct_whenCrossedWithItself_getZeroVector) {
        FPVector test(fp{-1}, fp{1}, fp{2});

        FPVector expected(fp{0}, fp{0}, fp{0});
        ASSERT_EQ(expected, test.cross(test));
    }

    TEST(right, returnsExpectedVector) {
        FPVector result = FPVector::right();
        
        FPVector expectedVec(fp{0}, fp{1}, fp{0});
        ASSERT_EQ(expectedVec, result);
    }

    TEST(distanceSq, whenPointsAreApart_returnsExpectedDistance) {
        FPVector firstVec(fp{10}, fp{10}, fp{10});
        FPVector secondVec(fp{100}, fp{100}, fp{100});

        fp expected = fp{24300};
        ASSERT_EQ(static_cast<float>(expected), static_cast<float>(FPVector::distanceSq(firstVec, secondVec)));
        ASSERT_EQ(static_cast<float>(expected), static_cast<float>(FPVector::distanceSq(secondVec, firstVec)));
    }

    TEST(distanceSq, whenPointsAreSame_returnsExpectedDistance) {
        FPVector firstVec(fp{-5}, fp{22.5f}, fp{-100});
        FPVector secondVec(fp{-5}, fp{22.5f}, fp{-100});

        fp expected = fp{0};
        ASSERT_EQ(static_cast<float>(expected), static_cast<float>(FPVector::distanceSq(firstVec, secondVec)));
        ASSERT_EQ(static_cast<float>(expected), static_cast<float>(FPVector::distanceSq(secondVec, firstVec)));
    }

    TEST(distance, whenPointsAreApart_returnsExpectedDistance) {
        FPVector firstVec(fp{10}, fp{10}, fp{10});
        FPVector secondVec(fp{100}, fp{100}, fp{100});

        fp expected = fp{155.88f};
        ASSERT_NEAR(static_cast<float>(expected), static_cast<float>(FPVector::distance(firstVec, secondVec)), 0.01f);
        ASSERT_NEAR(static_cast<float>(expected), static_cast<float>(FPVector::distance(secondVec, firstVec)), 0.01f);
    }

    TEST(distance, whenPointsAreSame_returnsExpectedDistance) {
        FPVector firstVec(fp{-5}, fp{22.5f}, fp{-100});
        FPVector secondVec(fp{-5}, fp{22.5f}, fp{-100});

        fp expected = fp{0};
        ASSERT_EQ(static_cast<float>(expected), static_cast<float>(FPVector::distance(firstVec, secondVec)));
        ASSERT_EQ(static_cast<float>(expected), static_cast<float>(FPVector::distance(secondVec, firstVec)));
    }

    TEST(directionNotNormalized, whenPointsAreApart_returnsExpectedDistance) {
        FPVector firstVec(fp{-5.5f}, fp{10}, fp{10});
        FPVector secondVec(fp{-5.5f}, fp{1010}, fp{10});

        FPVector expected = FPVector::right() * fp{1000};
        ASSERT_EQ(expected, FPVector::directionNotNormalized(firstVec, secondVec));
        ASSERT_EQ(expected * fp{-1}, FPVector::directionNotNormalized(secondVec, firstVec));
    }

    TEST(directionNotNormalized, whenPointsAreSame_returnsExpectedDistance) {
        FPVector firstVec(fp{-5}, fp{22.5f}, fp{-100});
        FPVector secondVec(fp{-5}, fp{22.5f}, fp{-100});

        FPVector expected = FPVector::zero();
        ASSERT_EQ(expected, FPVector::directionNotNormalized(firstVec, secondVec));
        ASSERT_EQ(expected, FPVector::directionNotNormalized(secondVec, firstVec));
    }

    TEST(direction, whenPointsAreApart_returnsExpectedDistance) {
        FPVector firstVec(fp{-5.5f}, fp{10}, fp{10});
        FPVector secondVec(fp{-5.5f}, fp{1010}, fp{10});

        FPVector expected = FPVector::right();
        ASSERT_EQ(expected, FPVector::direction(firstVec, secondVec));
        ASSERT_EQ(expected * fp{-1}, FPVector::direction(secondVec, firstVec));
    }

    TEST(direction, whenPointsAreSame_returnsExpectedDistance) {
        FPVector firstVec(fp{-5}, fp{22.5f}, fp{-100});
        FPVector secondVec(fp{-5}, fp{22.5f}, fp{-100});

        FPVector expected = FPVector::zero();
        ASSERT_EQ(expected, FPVector::direction(firstVec, secondVec));
        ASSERT_EQ(expected, FPVector::direction(secondVec, firstVec));
    }
    
    TEST(arrayOperator, getsEachValueAsExpected) {
        FPVector test(fp{-1}, fp{1}, fp{2});

        ASSERT_EQ(-1, static_cast<float>(test[0]));
        ASSERT_EQ(1, static_cast<float>(test[1]));
        ASSERT_EQ(2, static_cast<float>(test[2]));
    }
    
    TEST(arrayOperator, whenIndexOutOfRange_returnsMinLimitValue) {
        FPVector test(fp{-1}, fp{1}, fp{2});

        ASSERT_EQ(static_cast<float>(FPMath::minLimit()), static_cast<float>(test[3]));
        ASSERT_EQ(static_cast<float>(FPMath::minLimit()), static_cast<float>(test[-1]));
    }

    TEST(flipped, flipsSignOfAllValues) {
        FPVector input(fp{1}, fp{-1}, fp{0});
        FPVector result = input.flipped();

        ASSERT_EQ(input * fp{-1}, result);
    }

    TEST(flip, flipsSignOfAllValues) {
        FPVector test(fp{1}, fp{-1}, fp{0});
        FPVector expected = test * fp{-1};

        test.flip();
        ASSERT_EQ(expected, test);
    }

    TEST(isNear, whenBothVectorsAreSame_thenReturnsTrue) {
        FPVector first(fp{1}, fp{-1}, fp{0});
        FPVector second = first;

        fp tolerance = fp{0.1f};
        ASSERT_TRUE(first.isNear(second, tolerance));
    }

    TEST(isNear, whenVectorsDifferentAndWithinTolerance_thenReturnsTrue) {
        FPVector first(fp{1}, fp{-1}, fp{0});
        FPVector second(fp{0.95f}, fp{-1}, fp{0.1f});

        fp tolerance = fp{0.1f};
        ASSERT_TRUE(first.isNear(second, tolerance));
    }

    TEST(isNear, whenVectorsDifferentAndOutsideTolerance_thenReturnsFalse) {
        FPVector first(fp{1}, fp{-1}, fp{0});
        FPVector second(fp{0.95f}, fp{-1}, fp{0.5f});

        fp tolerance = fp{0.1f};
        ASSERT_FALSE(first.isNear(second, tolerance));
    }

    TEST(ostreamOutput, outputsCorrectStringForSimpleFPQuat) {
        FPVector vector(fp{-0.25f}, fp{1000}, fp{2.2f});

        std::stringstream out;
        out << vector;
 
        ASSERT_EQ("FPVector<-0.25, 1000, 2.2>", out.str());
    }

    // TODO: Constructors, zero vector, up, right, forward
    // TODO: FPVector ==, !=, and probably reversed mult.
    // TODO: normalize(), getLengthSquared()
    // TODO: FPVector dot and cross product with more "real"/"random" values
    // TODO: FPVector array operator with out of bounds
}
