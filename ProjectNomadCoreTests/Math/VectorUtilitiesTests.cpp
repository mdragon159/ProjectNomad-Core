#include "pch.h"
#include "Math/VectorUtilities.h"

using namespace ProjectNomad;

namespace VectorUtilitiesTests {
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

    // Parallel
    // Opposite parallel
    // Perpendicular
    // Partially in dir

    // IDEA: Non unit vector inputs? Non-direction vector input (ie, 0,0,0)?
}
