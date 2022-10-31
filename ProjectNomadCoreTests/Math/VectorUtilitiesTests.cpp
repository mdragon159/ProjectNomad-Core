#include "pchNCT.h"

#include "Math/FPEulerAngles.h"
#include "Math/FPMath2.h"
#include "Math/VectorUtilities.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace VectorUtilitiesTests {
    TEST(getAnyPerpendicularVector, whenGivenRightVector_returnsForwardVector) {
        FPVector input = FPVector::right();
        FPVector result = VectorUtilities::getAnyPerpendicularVector(input);

        TestHelpers::expectNear(FPVector::forward(), result, fp{0.1f});
    }

    TEST(getAnyPerpendicularVector, whenGivenUpVector_returnsBackwardVector) {
        FPVector input = FPVector::up();
        FPVector result = VectorUtilities::getAnyPerpendicularVector(input);

        TestHelpers::expectNear(FPVector::backward(), result, fp{0.1f});
    }

    TEST(getAnyPerpendicularVector, whenGivenDownVector_returnsForwardVector) {
        FPVector input = FPVector::down();
        FPVector result = VectorUtilities::getAnyPerpendicularVector(input);

        TestHelpers::expectNear(FPVector::forward(), result, fp{0.1f});
    }
    
    TEST(getParallelVectorProjection, whenGivenVectorParallelToDir_returnsInitialVector) {
        FPVector testVector(fp{0}, fp{250.5f}, fp{0});
        FPVector direction(fp{0}, fp{1}, fp{0});

        FPVector parallelComponentResult;
        bool isParallelOppDirResult;
        VectorUtilities::getParallelVectorProjection(testVector, direction, parallelComponentResult,
                                                     isParallelOppDirResult);

        ASSERT_EQ(testVector, parallelComponentResult);
        ASSERT_FALSE(isParallelOppDirResult);
    }

    TEST(getParallelVectorProjection, whenGivenVectorOppositeToDir_returnsInitialVector) {
        FPVector testVector(fp{0}, fp{-250.5f}, fp{0});
        FPVector direction(fp{0}, fp{1}, fp{0});

        FPVector parallelComponentResult;
        bool isParallelOppDirResult;
        VectorUtilities::getParallelVectorProjection(testVector, direction, parallelComponentResult,
                                                     isParallelOppDirResult);

        ASSERT_EQ(testVector, parallelComponentResult);
        ASSERT_TRUE(isParallelOppDirResult);
    }

    TEST(getParallelVectorProjection, whenGivenVectorPerpendicularToDir_returnsZeroVec) {
        FPVector testVector(fp{0}, fp{0}, fp{-125});
        FPVector direction(fp{1}, fp{0}, fp{0});

        FPVector parallelComponentResult;
        bool isParallelOppDirResult;
        VectorUtilities::getParallelVectorProjection(testVector, direction, parallelComponentResult,
                                                     isParallelOppDirResult);

        ASSERT_EQ(FPVector::zero(), parallelComponentResult);
        ASSERT_FALSE(isParallelOppDirResult);
    }

    TEST(getParallelVectorProjection, whenGivenVectorPartiallyToDir_returnsComponentInDir) {
        FPVector testVector(fp{125}, fp{0}, fp{-125});
        FPVector direction(fp{0}, fp{0}, fp{1});

        FPVector parallelComponentResult;
        bool isParallelOppDirResult;
        VectorUtilities::getParallelVectorProjection(testVector, direction, parallelComponentResult,
                                                     isParallelOppDirResult);

        ASSERT_EQ(FPVector(fp{0}, fp{0}, fp{-125}), parallelComponentResult);
        ASSERT_TRUE(isParallelOppDirResult);
    }

    TEST(getParallelVectorProjection_simplifiedOverload, whenVectorsPerpendicular_thenReturnsZeroVector) {
        FPVector testVector(fp{125}, fp{0}, fp{0});
        FPVector direction(fp{0}, fp{0}, fp{1});

        FPVector result;
        VectorUtilities::getParallelVectorProjection(testVector, direction, result);
        
        ASSERT_EQ(FPVector::zero(), result);
    }

    TEST(getParallelVectorProjection_simplifiedOverload, whenVectorsParallel_thenReturnsInitialVector) {
        FPVector testVector(fp{0}, fp{0}, fp{-125});
        FPVector direction(fp{0}, fp{0}, fp{1});

        FPVector result;
        VectorUtilities::getParallelVectorProjection(testVector, direction, result);
        
        ASSERT_EQ(testVector, result);
    }

    TEST(getParallelVectorProjection_simplifiedOverload, whenPartOfTestVectorIsInDirection_thenReturnsParallelComponent) {
        FPVector testVector(fp{125}, fp{200}, fp{-125});
        FPVector direction(fp{0}, fp{0}, fp{1});

        FPVector result;
        VectorUtilities::getParallelVectorProjection(testVector, direction, result);
        
        ASSERT_EQ(FPVector(fp{0}, fp{0}, fp{-125}), result);
    }

    TEST(getVectorsRelativeToDir_shorthand, whenGivenVectorParallelToDir_returnsExpectedResults) {
        FPVector testVector(fp{0}, fp{250.5f}, fp{0});
        FPVector direction(fp{0}, fp{1}, fp{0});

        FPVector parallelComponentResult, perpComponentResult;
        VectorUtilities::getVectorsRelativeToDir(testVector, direction,
                                                 parallelComponentResult, perpComponentResult);

        ASSERT_EQ(testVector, parallelComponentResult);
        ASSERT_EQ(FPVector::zero(), perpComponentResult);
    }

    TEST(getVectorsRelativeToDir, whenGivenVectorParallelToDir_returnsExpectedResults) {
        FPVector testVector(fp{0}, fp{250.5f}, fp{0});
        FPVector direction(fp{0}, fp{1}, fp{0});

        FPVector parallelComponentResult, perpComponentResult;
        bool isParallelOppDirResult;
        VectorUtilities::getVectorsRelativeToDir(testVector, direction,
                                                 parallelComponentResult, perpComponentResult, isParallelOppDirResult);

        ASSERT_EQ(testVector, parallelComponentResult);
        ASSERT_EQ(FPVector::zero(), perpComponentResult);
        ASSERT_FALSE(isParallelOppDirResult);
    }

    TEST(getVectorsRelativeToDir, whenGivenVectorOppositeToDir_returnsExpectedResults) {
        FPVector testVector(fp{0}, fp{-250.5f}, fp{0});
        FPVector direction(fp{0}, fp{1}, fp{0});

        FPVector parallelComponentResult, perpComponentResult;
        bool isParallelOppDirResult;
        VectorUtilities::getVectorsRelativeToDir(testVector, direction,
                                                 parallelComponentResult, perpComponentResult, isParallelOppDirResult);

        ASSERT_EQ(testVector, parallelComponentResult);
        ASSERT_EQ(FPVector::zero(), perpComponentResult);
        ASSERT_TRUE(isParallelOppDirResult);
    }

    TEST(getVectorsRelativeToDir, whenGivenVectorPerpendicularToDir_returnsExpectedResults) {
        FPVector testVector(fp{0}, fp{0}, fp{-125});
        FPVector direction(fp{1}, fp{0}, fp{0});

        FPVector parallelComponentResult, perpComponentResult;
        bool isParallelOppDirResult;
        VectorUtilities::getVectorsRelativeToDir(testVector, direction,
                                                 parallelComponentResult, perpComponentResult, isParallelOppDirResult);

        ASSERT_EQ(FPVector::zero(), parallelComponentResult);
        ASSERT_EQ(testVector, perpComponentResult);
        ASSERT_FALSE(isParallelOppDirResult);
    }

    TEST(getVectorsRelativeToDir, whenGivenVectorPartiallyToDir_returnsExpectedResults) {
        FPVector testVector(fp{125}, fp{200}, fp{-125});
        FPVector direction(fp{0}, fp{0}, fp{1});

        FPVector parallelComponentResult, perpComponentResult;
        bool isParallelOppDirResult;
        VectorUtilities::getVectorsRelativeToDir(testVector, direction,
                                                 parallelComponentResult, perpComponentResult, isParallelOppDirResult);

        ASSERT_EQ(FPVector(fp{0}, fp{0}, fp{-125}), parallelComponentResult);
        ASSERT_EQ(FPVector(fp{125}, fp{200}, fp{0}), perpComponentResult);
        ASSERT_TRUE(isParallelOppDirResult);
    }

    TEST(getAngleBetweenVectorsInDegrees, forwardComparison_whenSameDirection_returnsZero) {
        FPVector secondDir = FPVector::forward();

        fp result = VectorUtilities::getAngleBetweenVectorsInDegrees(FPVector::forward(), secondDir);

        TestHelpers::expectNear(fp{0}, result, fp{0.01f});
    }

    TEST(getAngleBetweenVectorsInDegrees, forwardComparison_whenToLeftDirection_returnsPositive90) {
        FPVector secondDir = FPVector::right() * fp{-1};

        fp result = VectorUtilities::getAngleBetweenVectorsInDegrees(FPVector::forward(), secondDir);

        TestHelpers::expectNear(fp{90}, result, fp{0.01f});
    }

    TEST(getAngleBetweenVectorsInDegrees, forwardComparison_whenToRightDirection_returnsPositive90) {
        FPVector secondDir = FPVector::right();

        fp result = VectorUtilities::getAngleBetweenVectorsInDegrees(FPVector::forward(), secondDir);

        TestHelpers::expectNear(fp{90}, result, fp{0.01f});
    }

    TEST(getAngleBetweenVectorsInDegrees, forwardComparison_whenOppositeDirection_returns180) {
        FPVector secondDir = FPVector::forward() * fp{-1};

        fp result = VectorUtilities::getAngleBetweenVectorsInDegrees(FPVector::forward(), secondDir);

        TestHelpers::expectNear(fp{180}, result, fp{0.01f});
    }

    TEST(getAngleBetweenVectorsInDegrees, forwardComparison_whenNearlyOppositeDirection_returnsExpectedValue) {
        EulerAngles eulerAngles;
        eulerAngles.yaw = fp{178};
        FPVector secondDir = FPMath2::eulerToDirVector(eulerAngles);

        fp result = VectorUtilities::getAngleBetweenVectorsInDegrees(FPVector::forward(), secondDir);

        TestHelpers::expectNear(fp{178}, result, fp{0.01f});
    }

    TEST(getAngleBetweenVectorsInDegrees, whenNormalizedDotProductBetweenVectorsIsSlightlyGreaterThanOne_thenStillReturnsExpectedValue) {
        // Real world case where first step in process results in a value SLIGHTLY greater than magnitude of one (-1.00001526).
        // This seems to have an imaginary component for inverse cosine and thus the underlying calculation previously failed.
        // Pass in the same values to the function to assure this edge case is fixed.
        FPVector firstDir = FPVector(fp{-0.191650f}, fp{-0.982468f}, fp{0});
        FPVector secondDir = FPVector(fp{0.190933f}, fp{0.981598f}, fp{0});

        fp result = VectorUtilities::getAngleBetweenVectorsInDegrees(firstDir, secondDir);

        TestHelpers::expectNear(fp{180}, result, fp{0.01f});
    }

    TEST(isXYCrossDotPositive, forwardComparison_whenSameDirection_returnsTrue) {
        FPVector secondDir = FPVector::forward();

        bool result = VectorUtilities::isXYCrossDotPositive(FPVector::forward(), secondDir);

        EXPECT_TRUE(result);
    }

    TEST(isXYCrossDotPositive, forwardComparison_whenToLeftDirection_returnsFalse) {
        FPVector secondDir = FPVector::right() * fp{-1};

        bool result = VectorUtilities::isXYCrossDotPositive(FPVector::forward(), secondDir);

        EXPECT_FALSE(result);
    }

    TEST(isXYCrossDotPositive, forwardComparison_whenToRightDirection_returnsTrue) {
        FPVector secondDir = FPVector::right();

        bool result = VectorUtilities::isXYCrossDotPositive(FPVector::forward(), secondDir);

        EXPECT_TRUE(result);
    }

    TEST(isXYCrossDotPositive, forwardComparison_whenOppositeDirection_returnsTrue) {
        FPVector secondDir = FPVector::forward() * fp{-1};

        bool result = VectorUtilities::isXYCrossDotPositive(FPVector::forward(), secondDir);

        EXPECT_TRUE(result);
    }

    TEST(IsDirectionCloseToHorizontal, whenHorizontalDirection_returnsTrue) {
        bool result = VectorUtilities::IsDirectionCloseToHorizontal(FPVector::forward(), fp{0});

        EXPECT_TRUE(result);
    }

    TEST(IsDirectionCloseToHorizontal, whenVerticalDirection_returnsFalse) {
        bool result = VectorUtilities::IsDirectionCloseToHorizontal(FPVector::up(), fp{30});

        EXPECT_FALSE(result);
    }

    TEST(IsDirectionCloseToHorizontal, whenDiagonalDirectionOutsideInputRange_returnsFalse) {
        FPVector checkDir = FPVector(fp{1}, fp{1}, fp{0.5f}).normalized();
        bool result = VectorUtilities::IsDirectionCloseToHorizontal(checkDir, fp{10});

        EXPECT_FALSE(result);
    }
    
    TEST(IsDirectionCloseToHorizontal, whenDiagonalDirectionWithinInputRange_returnsTrue) {
        FPVector checkDir = FPVector(fp{1}, fp{1}, fp{0.5f}).normalized();
        bool result = VectorUtilities::IsDirectionCloseToHorizontal(checkDir, fp{30});

        EXPECT_TRUE(result);
    }


    // Parallel
    // Opposite parallel
    // Perpendicular
    // Partially in dir

    // IDEA: Non unit vector inputs? Non-direction vector input (ie, 0,0,0)?
}
