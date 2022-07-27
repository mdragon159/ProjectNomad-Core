#include "pchNCT.h"

#include "TestHelpers/TestHelpers.h"
#include "Physics/Collider.h"
#include "Physics/ColliderHelpers.h"

using namespace ProjectNomad;
namespace ColliderTests {

    
#pragma region getFurthestPoint
     
    class Collider_GetFurthestPointTests : public BaseSimTest {
    protected:
        SimpleCollisions<TestLogger> simpleCollisions;
        ColliderHelpers<TestLogger> toTest;
        Collider collider;

        Collider_GetFurthestPointTests() : BaseSimTest(), simpleCollisions(testLogger), toTest(testLogger, simpleCollisions) {}
    };

    TEST_F(Collider_GetFurthestPointTests, whenNotInitializedCollider_returnsZeroVector) {
        FPVector result = toTest.getFurthestPoint(collider, FPVector::forward());
        ASSERT_EQ(FPVector::zero(), result);

        TestHelpers::verifyErrorsLogged(testLogger);
    }

    TEST_F(Collider_GetFurthestPointTests, sphere_whenProvidedAnyDirection_thenReturnsPointOnSurfaceInDirection) {
        collider.setSphere(FPVector(fp{5}, fp{4}, fp{3}), fp{5});
        
        FPVector result = toTest.getFurthestPoint(collider, FPVector::right() * fp{-1});
        ASSERT_EQ(FPVector(fp{5}, fp{-1}, fp{3}), result);
    }

    TEST_F(Collider_GetFurthestPointTests, box_whenProvidedUpDirection_whenNoRotation_thenReturnsFurthestUpwardsVertex) {
        FPVector center(fp{1}, fp{-2}, fp{0.5f});
        FPVector halfSize(fp{1}, fp{2}, fp{3});
        collider.setBox(center, halfSize);
        
        FPVector result = toTest.getFurthestPoint(collider, FPVector::up());

        // Result point can be anywhere along top face. Thus verify its within top face bounds
        TestHelpers::assertNear(fp{3.5f}, result.z, fp{0.01f}); // +z max
        TestHelpers::assertNear(fp{1}, result.x, fp{1});        // x axis extents
        TestHelpers::assertNear(fp{-2}, result.y, fp{2});       // y axis extents
    }

    TEST_F(Collider_GetFurthestPointTests, capsule_whenProvidedUpDirection_whenNoRotation_thenReturnsFurthestUpwardsPoint) {
        FPVector center(fp{1}, fp{-2}, fp{0.5f});
        fp radius = fp{2.5f};
        fp halfHeight = fp{30};
        collider.setCapsule(center, radius, halfHeight);
        
        FPVector result = toTest.getFurthestPoint(collider, FPVector::up());
        
        ASSERT_EQ(FPVector(fp{1}, fp{-2}, fp{30.5f}), result);
    }

    TEST_F(Collider_GetFurthestPointTests, capsule_whenProvidedDownDirection_whenNoRotation_thenReturnsFurthestDownwardsPoint) {
        FPVector center(fp{1}, fp{-2}, fp{0.5f});
        fp radius = fp{2.5f};
        fp halfHeight = fp{30};
        collider.setCapsule(center, radius, halfHeight);
        
        FPVector result = toTest.getFurthestPoint(collider, FPVector::up() * fp{-1});
        
        ASSERT_EQ(FPVector(fp{1}, fp{-2}, fp{-29.5f}), result);
    }

    TEST_F(Collider_GetFurthestPointTests, capsule_whenProvidedDiagonalDirection_whenNoRotation_thenReturnsFurthestDiagonalPoint) {
        FPVector center(fp{1}, fp{-2}, fp{0.5f});
        fp radius = fp{2.5f};
        fp halfHeight = fp{30};
        collider.setCapsule(center, radius, halfHeight);
        
        FPVector result = toTest.getFurthestPoint(collider, FPVector(fp{1}, fp{0}, fp{1}).normalized());

        TestHelpers::assertNear(FPVector(fp{2.76778f}, fp{-2}, fp{29.7678f}), result, fp{0.1f});
    }
    
#pragma endregion 
    
#pragma region Setters/"Constructors"

    TEST(constructor, createsNotInitializedCollider) {
        Collider collider;

        EXPECT_TRUE(collider.isNotInitialized());
        EXPECT_EQ(ColliderType::NotInitialized, collider.colliderType);
    }

    TEST(setBox, whenGivenNoRotation_createsExpectedBox) {
        FPVector center(fp{1}, fp{-2}, fp{0.5f});
        FPVector halfSize(fp{1}, fp{2}, fp{3});

        Collider collider;
        collider.setBox(center, halfSize);

        EXPECT_TRUE(collider.isBox());
        EXPECT_EQ(ColliderType::Box, collider.colliderType);

        EXPECT_EQ(center, collider.getCenter());
        EXPECT_EQ(center, collider.center);

        EXPECT_EQ(halfSize, collider.getBoxHalfSize());
    }

    TEST(setBox, whenGivenRotation_createsExpectedBox) {
        FPVector center(fp{1}, fp{-2}, fp{0.5f});
        FPVector halfSize(fp{1}, fp{2}, fp{3});
        FPQuat rotation = FPQuat::fromDegrees(FPVector(fp{0}, fp{-1}, fp{0}).normalized(), fp{45});

        Collider collider;
        collider.setBox(center, rotation, halfSize);

        ASSERT_TRUE(collider.isBox());
        ASSERT_EQ(ColliderType::Box, collider.colliderType);

        ASSERT_EQ(center, collider.getCenter());
        ASSERT_EQ(center, collider.center);
        ASSERT_EQ(rotation, collider.getRotation());
        ASSERT_EQ(rotation, collider.rotation);

        ASSERT_EQ(halfSize, collider.getBoxHalfSize());
    }

    TEST(setCapsule, whenGivenNoRotation_createsExpectedCollider) {
        FPVector center(fp{1}, fp{-2}, fp{0.5f});
        fp radius = fp{2.5f};
        fp halfHeight = fp{30};

        Collider collider;
        collider.setCapsule(center, radius, halfHeight);

        ASSERT_TRUE(collider.isCapsule());
        ASSERT_EQ(ColliderType::Capsule, collider.colliderType);

        ASSERT_EQ(center, collider.getCenter());
        ASSERT_EQ(center, collider.center);

        ASSERT_EQ(radius, collider.getCapsuleRadius());
        ASSERT_EQ(radius, collider.radius);
        ASSERT_EQ(halfHeight, collider.getCapsuleHalfHeight());
        ASSERT_EQ(halfHeight, collider.capsuleHalfHeight);
    }

    TEST(setCapsule, whenGivenRotation_createsExpectedCollider) {
        FPVector center(fp{1}, fp{-2}, fp{0.5f});
        fp radius = fp{2.5f};
        fp halfHeight = fp{30};
        FPQuat rotation = FPQuat::fromDegrees(FPVector(fp{0}, fp{-1}, fp{0}).normalized(), fp{45});

        Collider collider;
        collider.setCapsule(center, rotation, radius, halfHeight);

        ASSERT_TRUE(collider.isCapsule());
        ASSERT_EQ(ColliderType::Capsule, collider.colliderType);

        ASSERT_EQ(center, collider.getCenter());
        ASSERT_EQ(center, collider.center);
        ASSERT_EQ(rotation, collider.getRotation());
        ASSERT_EQ(rotation, collider.rotation);

        ASSERT_EQ(radius, collider.getCapsuleRadius());
        ASSERT_EQ(radius, collider.radius);
        ASSERT_EQ(halfHeight, collider.getCapsuleHalfHeight());
        ASSERT_EQ(halfHeight, collider.capsuleHalfHeight);
    }

    TEST(setSphere, createsExpectedCollider) {
        FPVector center(fp{1}, fp{-2}, fp{0.5f});
        fp radius = fp{2.5f};

        Collider collider;
        collider.setSphere(center, radius);

        ASSERT_TRUE(collider.isSphere());
        ASSERT_EQ(ColliderType::Sphere, collider.colliderType);

        ASSERT_EQ(center, collider.getCenter());
        ASSERT_EQ(center, collider.center);

        ASSERT_EQ(radius, collider.getSphereRadius());
        ASSERT_EQ(radius, collider.radius);
    }

#pragma endregion
#pragma region Shared Utility Functions

    TEST(toWorldSpaceFromLocal, whenCollierDoesNotHaveRotation_reCentersPoint) {
        FPVector center(fp{1}, fp{0}, fp{0});
        FPVector halfSize(fp{1}, fp{2}, fp{3});
        FPQuat rotation = FPQuat::identity();
        Collider collider;
        collider.setBox(center, rotation, halfSize);

        FPVector localPoint(fp{2}, fp{0}, fp{0});
        FPVector result = collider.toWorldSpaceFromLocal(localPoint);

        FPVector expected(fp{3}, fp{0}, fp{0});
        TestHelpers::assertNear(expected, result, fp{0.01f});
    }

    TEST(toWorldSpaceFromLocal, whenCollierHasRotation_rotatesPoint) {
        FPVector center(fp{1}, fp{0}, fp{0});
        FPVector halfSize(fp{1}, fp{2}, fp{3});
        FPQuat rotation = FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}).normalized(), fp{90});
        Collider collider;
        collider.setBox(center, rotation, halfSize);

        FPVector localPoint(fp{0}, fp{-2}, fp{0});
        FPVector result = collider.toWorldSpaceFromLocal(localPoint);

        FPVector expected(fp{3}, fp{0}, fp{0});
        TestHelpers::assertNear(expected, result, fp{0.01f});
    }

    TEST(toLocalSpaceFromWorld, whenCollierDoesNotHaveRotation_reCentersPoint) {
        FPVector center(fp{1}, fp{0}, fp{0});
        FPVector halfSize(fp{1}, fp{2}, fp{3});
        FPQuat rotation = FPQuat::identity();
        Collider collider;
        collider.setBox(center, rotation, halfSize);

        FPVector worldPoint(fp{3}, fp{0}, fp{0});
        FPVector result = collider.toLocalSpaceFromWorld(worldPoint);

        FPVector expected(fp{2}, fp{0}, fp{0});
        TestHelpers::assertNear(expected, result, fp{0.01f});
    }

    TEST(toLocalSpaceFromWorld, whenCollierHasRotation_rotatesPoint) {
        FPVector center(fp{1}, fp{0}, fp{0});
        FPVector halfSize(fp{1}, fp{2}, fp{3});
        FPQuat rotation = FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}).normalized(), fp{90});
        Collider collider;
        collider.setBox(center, rotation, halfSize);

        FPVector worldPoint(fp{3}, fp{0}, fp{0});
        FPVector result = collider.toLocalSpaceFromWorld(worldPoint);

        FPVector expected(fp{0}, fp{-2}, fp{0});
        TestHelpers::assertNear(expected, result, fp{0.01f});
    }

    TEST(toString, whenNotInitialized_returnsExpectedString) {
        Collider collider;
        ASSERT_EQ("<Not Initialized Collider>", collider.toString());
    }

    TEST(toString, whenSphere_returnsExpectedString) {
        Collider collider;
        collider.setSphere(FPVector::zero(), fp{5});

        ASSERT_EQ("Sphere center: {x: 0.000000 | y: 0.000000 | z: 0.000000}, radius: {5.000000}",
            collider.toString());
    }

    TEST(getTypeAsString, whenNotInitialized_returnsExpectedString) {
        Collider collider;
        ASSERT_EQ("<Not Initialized Collider>", collider.getTypeAsString());
    }

    TEST(getTypeAsString, whenBox_returnsExpectedString) {
        Collider collider;
        collider.setBox(FPVector::zero(), {fp{1}, fp{2}, fp{3}});

        ASSERT_EQ("Box", collider.getTypeAsString());
    }

    TEST(getTypeAsString, whenCapsule_returnsExpectedString) {
        Collider collider;
        collider.setCapsule(FPVector::zero(), fp{5}, fp{5});

        ASSERT_EQ("Capsule", collider.getTypeAsString());
    }

    TEST(getTypeAsString, whenSphere_returnsExpectedString) {
        Collider collider;
        collider.setSphere(FPVector::zero(), fp{5});

        ASSERT_EQ("Sphere", collider.getTypeAsString());
    }

    TEST(ostreamOutput, outputsToStringWrappedInTypeName) {
        Collider collider;

        std::stringstream out;
        out << collider;

        ASSERT_EQ("Collider<<Not Initialized Collider>>", out.str());
    }

#pragma endregion

#pragma region Box Specific Functionality

    TEST(getBoxVerticesInWorldCoordinates, whenSimpleNoRotationBox_returnsExpectedVertices) {
        Collider collider;
        collider.setBox(FPVector::zero(), FPVector(fp{1}, fp{1}, fp{1}));

        std::vector<FPVector> result = collider.getBoxVerticesInWorldCoordinates();

        FPVector bottomBackLeft(fp{-1}, fp{-1}, fp{-1});
        FPVector bottomBackRight(fp{-1}, fp{1}, fp{-1});
        FPVector bottomFrontLeft(fp{1}, fp{-1}, fp{-1});
        FPVector bottomFrontRight(fp{1}, fp{1}, fp{-1});
        FPVector frontBackLeft(fp{-1}, fp{-1}, fp{1});
        FPVector frontBackRight(fp{-1}, fp{1}, fp{1});
        FPVector frontFrontLeft(fp{1}, fp{-1}, fp{1});
        FPVector frontFrontRight(fp{1}, fp{1}, fp{1});

        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomBackLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomBackRight) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomFrontLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomFrontRight) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontBackLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontBackRight) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontFrontLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontFrontRight) != result.end());
    }

    TEST(getBoxVerticesInWorldCoordinates, whenSimpleRotatedBox_returnsExpectedVertices) {
        FPQuat rotation = FPQuat::fromDegrees({fp{0}, fp{0}, fp{1}}, fp{90}); // rotate 90 around z axis
        Collider collider;
        collider.setBox(FPVector::zero(), rotation, FPVector(fp{1}, fp{2}, fp{1}));

        std::vector<FPVector> result = collider.getBoxVerticesInWorldCoordinates();

        // Rotate the expected points instead of calculating by hand
        // TODO: Rename all these points as now inaccurate
        FPVector bottomBackLeft = rotation * FPVector(fp{-1}, fp{-2}, fp{-1});
        FPVector bottomBackRight = rotation * FPVector(fp{-1}, fp{2}, fp{-1});
        FPVector bottomFrontLeft = rotation * FPVector(fp{1}, fp{-2}, fp{-1});
        FPVector bottomFrontRight = rotation * FPVector(fp{1}, fp{2}, fp{-1});
        FPVector frontBackLeft = rotation * FPVector(fp{-1}, fp{-2}, fp{1});
        FPVector frontBackRight = rotation * FPVector(fp{-1}, fp{2}, fp{1});
        FPVector frontFrontLeft = rotation * FPVector(fp{1}, fp{-2}, fp{1});
        FPVector frontFrontRight = rotation * FPVector(fp{1}, fp{2}, fp{1});

        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomBackLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomBackRight) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomFrontLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomFrontRight) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontBackLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontBackRight) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontFrontLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontFrontRight) != result.end());
    }

    TEST(getBoxVerticesInWorldCoordinates, whenNotCenteredNoRotationBox_returnsExpectedVertices) {
        FPVector center(fp{0.5}, fp{-1}, fp{2});
        Collider collider;
        collider.setBox(center, FPVector(fp{1}, fp{1}, fp{1}));

        std::vector<FPVector> result = collider.getBoxVerticesInWorldCoordinates();
        
        FPVector bottomBackLeft = FPVector(fp{-1}, fp{-1}, fp{-1}) + center;
        FPVector bottomBackRight = FPVector(fp{-1}, fp{1}, fp{-1}) + center;
        FPVector bottomFrontLeft = FPVector(fp{1}, fp{-1}, fp{-1}) + center;
        FPVector bottomFrontRight = FPVector(fp{1}, fp{1}, fp{-1}) + center;
        FPVector frontBackLeft = FPVector(fp{-1}, fp{-1}, fp{1}) + center;
        FPVector frontBackRight = FPVector(fp{-1}, fp{1}, fp{1}) + center;
        FPVector frontFrontLeft = FPVector(fp{1}, fp{-1}, fp{1}) + center;
        FPVector frontFrontRight = FPVector(fp{1}, fp{1}, fp{1}) + center;

        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomBackLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomBackRight) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomFrontLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), bottomFrontRight) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontBackLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontBackRight) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontFrontLeft) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), frontFrontRight) != result.end());
    }

    TEST(getBoxNormalsInWorldCoordinates, whenNoRotationBox_returnsExpectedNormals) {
        Collider collider;
        collider.setBox(FPVector::zero(), FPVector(fp{1}, fp{1}, fp{1}));

        std::vector<FPVector> result = collider.getBoxNormalsInWorldCoordinates();

        FPVector originalXAxis = FPVector(fp{1}, fp{0}, fp{0});
        FPVector originalYAxis = FPVector(fp{0}, fp{1}, fp{0});
        FPVector originalZAxis = FPVector(fp{0}, fp{0}, fp{1});

        ASSERT_TRUE(std::find(result.begin(), result.end(), originalXAxis) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), originalYAxis) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), originalZAxis) != result.end());
    }

    TEST(getBoxNormalsInWorldCoordinates, whenSimpleRotatedBox_returnsExpectedNormals) {
        FPQuat rotation = FPQuat::fromDegrees({fp{0}, fp{0}, fp{1}}, fp{90});
        FPVector center(fp{0.5}, fp{-1}, fp{2});
        Collider collider;
        collider.setBox(center, rotation, FPVector(fp{1}, fp{1}, fp{1}));

        std::vector<FPVector> result = collider.getBoxNormalsInWorldCoordinates();

        FPVector originalXAxis = rotation * FPVector(fp{1}, fp{0}, fp{0});
        FPVector originalYAxis = rotation * FPVector(fp{0}, fp{1}, fp{0});
        FPVector originalZAxis = FPVector(fp{0}, fp{0}, fp{1});

        ASSERT_TRUE(std::find(result.begin(), result.end(), originalXAxis) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), originalYAxis) != result.end());
        ASSERT_TRUE(std::find(result.begin(), result.end(), originalZAxis) != result.end());
    }

    TEST(isWorldSpacePtWithinBoxIncludingOnSurface, whenNoRotationBox_givenPointInsideBox_returnsTrue) {
        Collider collider;
        collider.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{1}, fp{1}, fp{1})));
        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{1.5f}, fp{0.5f}, fp{0.75f})));
    }

    TEST(isWorldSpacePtWithinBoxIncludingOnSurface, whenNoRotationBox_givenPointOnSurface_returnsTrue) {
        Collider collider;
        collider.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{0}, fp{0}, fp{0})));
        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{2}, fp{2}, fp{2})));
        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{1}, fp{2}, fp{2})));
        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{0}, fp{0}, fp{1.5f})));
        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{1.5f}, fp{0.5f}, fp{0})));
    }

    TEST(isWorldSpacePtWithinBoxIncludingOnSurface, whenNoRotationBox_givenPointOutsideBox_returnsFalse) {
        Collider collider;
        collider.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{-1}, fp{2}, fp{2})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{-1}, fp{0}, fp{0})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{-1}, fp{-1}, fp{-1})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{2.1f}, fp{2}, fp{2})));
    }

    TEST(isWorldSpacePtWithinBoxIncludingOnSurface, whenRotatedBox_returnsExpectedResults) {
        FPQuat rotation = FPQuat::fromDegrees({fp{0}, fp{0}, fp{1}}, fp{90});
        Collider collider;
        collider.setBox(FPVector(fp{1}, fp{1}, fp{1}), rotation, FPVector(fp{0.5f}, fp{1}, fp{1}));

        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{1}, fp{1}, fp{2})));
        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{1.9f}, fp{1.4f}, fp{1.9f})));

        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{0}, fp{0}, fp{0})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{1}, fp{0}, fp{1})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxIncludingOnSurface(FPVector(fp{1.4f}, fp{1.9f}, fp{1.9f})));
    }

    TEST(isWorldSpacePtWithinBoxExcludingOnSurface, whenNoRotationBox_givenPointInsideBox_returnsTrue) {
        Collider collider;
        collider.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{1}, fp{1}, fp{1})));
        EXPECT_TRUE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{1.5f}, fp{0.5f}, fp{0.75f})));
    }

    TEST(isWorldSpacePtWithinBoxExcludingOnSurface, whenNoRotationBox_givenPointOnSurface_returnsFalse) {
        Collider collider;
        collider.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{0}, fp{0}, fp{0})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{2}, fp{2}, fp{2})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{1}, fp{2}, fp{2})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{0}, fp{0}, fp{1.5f})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{1.5f}, fp{0.5f}, fp{0})));
    }

    TEST(isWorldSpacePtWithinBoxExcludingOnSurface, whenNoRotationBox_givenPointOutsideBox_returnsFalse) {
        Collider collider;
        collider.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{-1}, fp{2}, fp{2})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{-1}, fp{0}, fp{0})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{-1}, fp{-1}, fp{-1})));
        EXPECT_FALSE(collider.isWorldSpacePtWithinBoxExcludingOnSurface(FPVector(fp{2.1f}, fp{2}, fp{2})));
    }
    
#pragma endregion
#pragma region Capsule Specific Functionality

    TEST(getMedialHalfLineLength, whenRadiusAndHalfHeightEqual_thenReturnsZero) {
        Collider collider;
        collider.setCapsule(FPVector::zero(), fp{5}, fp{5});

        ASSERT_EQ(fp{0}, collider.getMedialHalfLineLength());
    }

    TEST(getMedialHalfLineLength, whenHalfHeightGreaterThanRAdius_thenReturnsCorrectLength) {
        Collider collider;
        collider.setCapsule(FPVector::zero(), fp{5}, fp{20});

        ASSERT_EQ(fp{15}, collider.getMedialHalfLineLength());
    }

    TEST(getCapsuleMedialLineExtremes, whenRadiusAndHalfHeightEqual_thenReturnsExpectedLine) {
        FPVector center(fp{-1}, fp{2}, fp{3});
        Collider collider;
        collider.setCapsule(center, fp{5}, fp{5});

        Line result = collider.getCapsuleMedialLineExtremes();
        ASSERT_EQ(center, result.start);
        ASSERT_EQ(center, result.end);
    }

    TEST(getCapsuleMedialLineExtremes, whenHalfHeightGreaterThanRAdius_thenReturnsExpectedLine) {
        Collider collider;
        collider.setCapsule(FPVector::zero(), fp{5}, fp{20});

        Line result = collider.getCapsuleMedialLineExtremes();
        ASSERT_EQ(FPVector(fp{0}, fp{0}, fp{-15}), result.start);
        ASSERT_EQ(FPVector(fp{0}, fp{0}, fp{15}), result.end);
    }
    
#pragma endregion 
}