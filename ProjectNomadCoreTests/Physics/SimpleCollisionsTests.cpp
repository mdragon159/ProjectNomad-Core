#include "pchNCT.h"

#include "TestHelpers/TestHelpers.h"
#include "Physics/Collider.h"
#include "Physics/PhysicsManager.h"
#include "Physics/SimpleCollisions.h"

using namespace ProjectNomad;

namespace SimpleCollisionsTests {
    class SimpleCollisionsFixtureBase : public BaseSimTest {
    protected:
        PhysicsManager<TestLogger> physicsManager;
        SimpleCollisions<TestLogger>& simpleCollisions;
        
        Collider colliderA;
        Collider colliderB;

        SimpleCollisionsFixtureBase()
        : BaseSimTest(), physicsManager(testLogger), simpleCollisions(physicsManager.getSimpleCollisions()) {}
    };

#pragma region Wrong type checks
    class SimpleWrongTypeErrors : public SimpleCollisionsFixtureBase {
    protected:
        void TearDown() override {
            TestHelpers::verifyErrorsLogged(testLogger);
        }
    };

    TEST_F(SimpleWrongTypeErrors, isColliding_whenFirstColliderNotInitialized_thenLogsError) {
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isColliding_whenSecondColliderNotInitialized_thenLogsError) {
        colliderA.setSphere(FPVector::zero(), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isBoxAndBoxColliding_whenFirstColliderNotInitialized_thenLogsError) {
        bool result = simpleCollisions.isBoxAndBoxColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isBoxAndBoxColliding_whenSecondColliderNotInitialized_thenLogsError) {
        colliderA.setBox(FPVector::zero(), FPVector(fp{5}));

        bool result = simpleCollisions.isBoxAndBoxColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isCapsuleAndCapsuleColliding_whenFirstColliderNotInitialized_thenLogsError) {
        bool result = simpleCollisions.isCapsuleAndCapsuleColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isCapsuleAndCapsuleColliding_whenSecondColliderNotInitialized_thenLogsError) {
        colliderA.setCapsule(FPVector::zero(), fp{5}, fp{10});

        bool result = simpleCollisions.isCapsuleAndCapsuleColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isSphereAndSphereColliding_whenFirstColliderNotInitialized_thenLogsError) {
        bool result = simpleCollisions.isSphereAndSphereColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isSphereAndSphereColliding_whenSecondColliderNotInitialized_thenLogsError) {
        colliderA.setSphere(FPVector::zero(), fp{5});

        bool result = simpleCollisions.isSphereAndSphereColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isBoxAndCapsuleColliding_whenFirstColliderNotInitialized_thenLogsError) {
        bool result = simpleCollisions.isBoxAndCapsuleColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isBoxAndCapsuleColliding_whenSecondColliderNotInitialized_thenLogsError) {
        colliderA.setBox(FPVector::zero(), FPVector(fp{5}));

        bool result = simpleCollisions.isBoxAndCapsuleColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isBoxAndSphereColliding_whenFirstColliderNotInitialized_thenLogsError) {
        bool result = simpleCollisions.isBoxAndSphereColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isBoxAndSphereColliding_whenSecondColliderNotInitialized_thenLogsError) {
        colliderA.setBox(FPVector::zero(), FPVector(fp{5}));

        bool result = simpleCollisions.isBoxAndSphereColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isCapsuleAndSphereColliding_whenFirstColliderNotInitialized_thenLogsError) {
        bool result = simpleCollisions.isCapsuleAndSphereColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, isCapsuleAndSphereColliding_whenSecondColliderNotInitialized_thenLogsError) {
        colliderA.setCapsule(FPVector::zero(), fp{5}, fp{10});

        bool result = simpleCollisions.isCapsuleAndSphereColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, raycastWithSphere_whenColliderNotInitialized_thenLogsError) {
        fp throwawayFp;
        FPVector throwawayVector;
        bool result = simpleCollisions.raycastWithSphere(Ray(), colliderA, throwawayFp, throwawayVector);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, raycastWithBox_whenColliderNotInitialized_thenLogsError) {
        fp throwawayFp;
        FPVector throwawayVector;
        bool result = simpleCollisions.raycastWithBox(Ray(), colliderA, throwawayFp, throwawayVector);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, linetestWithBox_whenColliderNotInitialized_thenLogsError) {
        fp throwawayFp;
        FPVector throwawayVector;
        bool result = simpleCollisions.linetestWithBox(Line(), colliderA, throwawayFp, throwawayVector);
        EXPECT_FALSE(result);
    }

    TEST_F(SimpleWrongTypeErrors, linetestWithCapsule_whenColliderNotInitialized_thenLogsError) {
        fp throwawayFp;
        FPVector throwawayVector;
        bool result = simpleCollisions.linetestWithCapsule(Line(), colliderA, throwawayFp, throwawayVector);
        EXPECT_FALSE(result);
    }

#pragma endregion

#pragma region isColliding: Box and Box

    class BoxBoxCollisions : public SimpleCollisionsFixtureBase {};

    TEST_F(BoxBoxCollisions, whenBoxesAreDistant_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}));
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxBoxCollisions, whenTouching_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxBoxCollisions, whenIntersecting_statesIsColliding) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxBoxCollisions, whenRotatedOBBs_whenNotIntersecting_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{1}, fp{1}, fp{1})
        );
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxBoxCollisions, whenRotatedOBBs_whenIntersecting_statesIsColliding) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(
        FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}),
        FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{1}, fp{1}, fp{1})
        );
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }
    
#pragma endregion
#pragma region isColliding: Capsule and Capsule

    class CapsuleCapsuleCollisions : public SimpleCollisionsFixtureBase {};

    TEST_F(CapsuleCapsuleCollisions, whenIdenticalOverlappingColliders_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(CapsuleCapsuleCollisions, whenBarelyCollidingOnEnds_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{39.9f}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(CapsuleCapsuleCollisions, whenJustTouchingOnEnds_notColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{40}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(CapsuleCapsuleCollisions, whenNowhereClose_notColliding) {
        colliderA.setCapsule(FPVector(fp{100}, fp{-30}, fp{40}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{-5}, fp{20.5f}, fp{100}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(CapsuleCapsuleCollisions, whenIdenticalSizedOverlappingColliders_withSingleRotation_statesIsColliding) {
        FPQuat rotationAroundX = FPQuat::fromDegrees(FPVector(fp{1}, fp{0}, fp{0}), fp{90});
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), rotationAroundX, fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(CapsuleCapsuleCollisions, whenBarelyCollidingOnOneEnd_withRotations_statesIsColliding) {
        FPQuat rotationAroundX = FPQuat::fromDegrees(FPVector(fp{1}, fp{0}, fp{0}), fp{90});
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{29.9f}), rotationAroundX, fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(CapsuleCapsuleCollisions, whenJustTouchingOnOneEnd_withRotations_notColliding) {
        FPQuat rotationAroundX = FPQuat::fromDegrees(FPVector(fp{1}, fp{0}, fp{0}), fp{90});
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{30}), rotationAroundX, fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(CapsuleCapsuleCollisions, whenNowhereClose_withRotations_notColliding) {
        FPQuat rotationAroundX = FPQuat::fromDegrees(FPVector(fp{1}, fp{0}, fp{0}), fp{90});
        colliderA.setCapsule(FPVector(fp{100}, fp{-30}, fp{40}), rotationAroundX, fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{-5}, fp{20.5f}, fp{100}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }
    
#pragma endregion
#pragma region isColliding: Sphere and Sphere

    class SphereSphereCollisions : public SimpleCollisionsFixtureBase {};

    TEST_F(SphereSphereCollisions, whenDistantSpheres_statesNotColliding) {
        colliderA.setSphere(FPVector(fp{-20}, fp{5}, fp{10}), fp{5});
        colliderB.setSphere(FPVector(fp{20}, fp{0.5f}, fp{0}), fp{1});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SphereSphereCollisions, whenSpheresTouching_statesNotColliding) {
        colliderA.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        colliderB.setSphere(FPVector(fp{5}, fp{0}, fp{0}), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(SphereSphereCollisions, whenSpheresIntersecting_statesIsColliding) {
        colliderA.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        colliderB.setSphere(FPVector(fp{5}, fp{0}, fp{0}), fp{6});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

#pragma endregion
#pragma region isColliding: Box and Capsule

    class BoxCapsuleCollisions : public SimpleCollisionsFixtureBase {};

    TEST_F(BoxCapsuleCollisions, whenCenteredOverlappingColliders_whenMedialLineWithinExpandedBox_statesIsColliding) {
        // Case where median line is inside expanded box (box half size increased by radius)
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenCenteredOverlappingColliders_whenMedianLineCrossesExpandedBox_statesIsColliding) {
        // Case where median line is inside expanded box (box half size increased by radius)
        colliderB.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{100});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenBarelyIntersectingOnOneEndOfCapsule_statesIsColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{23.9f}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenTouchingOnOneEndOfCapsule_fromAbove_notColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{24}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenTouchingOnOneEndOfCapsule_fromBelow_notColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{-24}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenTouchingOnSideOfCapsule_fromBack_notColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{-14}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenTouchingOnSideOfCapsule_fromFront_notColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{14}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenBoxToSideOfCapsuleEndSphere_givenBoxOutsideSphereButWithinExtendedBox_notColliding) {
        // Set up case from manual testing in interactable tool where box is clearly not touching top of capsule's sphere,
        //  but would be touching a box which covers the capsule (ie, not treating ends of capsule as spheres)
        colliderA.setCapsule(FPVector(fp{120}, fp{-4}, fp{182}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{120}, fp{-77}, fp{281}), FPVector(fp{50}));
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenBoxToSideOfCapsule_givenCapsuleMedianLineNotInsideBoxButColliding_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{120}, fp{-4}, fp{182}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{120}, fp{52}, fp{204}), FPVector(fp{50}));
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenTinyBoxToSideOfCapsule_givenCapsuleMedianLineNotInsideBoxButColliding_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{120}, fp{-4}, fp{182}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{120}, fp{21}, fp{184}), FPVector(fp{5}));
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenLargeBoxTouchingTopHemisphereOfCapsule_givenBoxNotTouchingMedianLine_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{120}, fp{-4}, fp{182}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{121}, fp{-58}, fp{102}), FPVector(fp{50}));
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenLargeBoxTouchingRightSideOfCapsule_givenNotTouchingMedialLineAndCapsuleHigherThanRadius_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{-51}, fp{26}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{50}));
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenCapsuleTouchingFrontSurfaceOfLargeBox_statesNotColliding) {
        // This test has the median line running parallel to box face and only touching the face,
        //      which we've chosen to not consider as a collision
        colliderA.setCapsule(FPVector(fp{-75}, fp{-8}, fp{54}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{50}));

        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        ASSERT_FALSE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenRotatedCapsuleIntersectsBoxCorner_statesIsColliding) {
        FPQuat capsuleRotation = FPQuat::fromDegrees(FPVector::left(), fp{30});
        colliderA.setCapsule(FPVector(fp{75}, fp{8}, fp{54}), capsuleRotation, fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{50}));

        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result);
    }

    TEST_F(BoxCapsuleCollisions, whenDistant_notColliding) {
        colliderA.setBox(FPVector(fp{200}, fp{33.3f}, fp{-5}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{5}, fp{-10}, fp{24}), fp{10}, fp{20});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        ASSERT_FALSE(result);
    }

#pragma endregion
#pragma region isColliding: Box and Sphere

    class BoxSphereCollisions : public SimpleCollisionsFixtureBase {};

    TEST_F(BoxSphereCollisions, givenNoRotationBox_givenSphereDistantFromBox_statesNotColliding) {
        colliderB.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderA.setSphere(FPVector(fp{-5}, fp{5}, fp{10}), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxSphereCollisions, givenNoRotationBox_givenSphereTouchingBox_statesNotColliding) {
        colliderA.setBox(FPVector(fp{1}, fp{0}, fp{0}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(BoxSphereCollisions, givenNoRotationBox_givenSphereIntersectingBox_statesIsColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxSphereCollisions, givenNoRotationBox_givenSphereCenteredWithinBox_statesIsColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{2}, fp{2}, fp{2}));
        colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{0}), fp{1});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(BoxSphereCollisions, givenNoRotationBox_givenSphereWithinBoxButNotCentered_statesIsColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{3}, fp{3}, fp{3}));
        colliderB.setSphere(FPVector(fp{1}, fp{1}, fp{1}), fp{1});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

#pragma endregion
#pragma region isColliding: Capsule and Sphere

    class CapsuleSphereCollisions : public SimpleCollisionsFixtureBase {};

    TEST_F(CapsuleSphereCollisions, whenCenteredOverlappingColliders_statesIsColliding) {
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderA.setSphere(FPVector(fp{0}, fp{0}, fp{0}), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(CapsuleSphereCollisions, whenBarelyTouchingOnTopOfCapsule_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{24.9f}), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(CapsuleSphereCollisions, whenJustTouchingOnTopOfCapsule_notColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{5}), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(CapsuleSphereCollisions, whenBarelyTouchingSideOfCapsule_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{14.5f}, fp{0}), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(CapsuleSphereCollisions, whenJustTouchingSideOfCapsule_notColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{15}, fp{0}), fp{5});
        
        bool result = simpleCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

#pragma endregion 

#pragma region Raycast vs Sphere

    class RaycastWithSphereTests : public SimpleCollisionsFixtureBase {};

    TEST_F(RaycastWithSphereTests, givenSimpleRayInsideSphere_returnsIntersectionTime) {
        Ray ray(FPVector::zero(), FPVector(fp{0}, fp{1}, fp{0}));
        colliderA.setSphere(FPVector::zero(), fp{3});

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithSphere(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        EXPECT_EQ(fp{3}, timeOfIntersection);
        EXPECT_EQ(FPVector(fp{0}, fp{3}, fp{0}), pointOfIntersection);
    }
    
    TEST_F(RaycastWithSphereTests, whenRayOutsideSphere_whenRayPointsTowardsSphere_returnsIntersectionTime) {
        Ray ray(FPVector(fp{0}, fp{-1}, fp{0}), FPVector(fp{0}, fp{1}, fp{0}));
        colliderA.setSphere(FPVector(fp{3}, fp{3}, fp{0}), fp{3});
    
        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithSphere(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        EXPECT_EQ(fp{4}, timeOfIntersection);
        EXPECT_EQ(FPVector(fp{0}, fp{3}, fp{0}), pointOfIntersection);
    }
    
    TEST_F(RaycastWithSphereTests, whenRayOutsideSphere_whenRayPointsOppositeOfSphere_returnsNegativeIntersectionTime) {
        Ray ray(FPVector(fp{0}, fp{-1}, fp{0}), FPVector(fp{0}, fp{-1}, fp{0}));
        colliderA.setSphere(FPVector(fp{3}, fp{3}, fp{0}), fp{3});
    
        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithSphere(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }
    
    TEST_F(RaycastWithSphereTests, whenRayOutsideSphere_whenRayPointsAwayFromSphere_returnsNoIntersection) {
        Ray ray(FPVector(fp{0}, fp{-1}, fp{0}), FPVector(fp{1}, fp{0}, fp{0}));
        colliderA.setSphere(FPVector(fp{3}, fp{3}, fp{0}), fp{3});
    
        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithSphere(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }

    // TODO: Unit test when ray crosses sphere surface. Current chosen standard is simply touching surface should NOT count as intersection

#pragma endregion
#pragma region Raycast vs Box

    class RaycastWithBoxTests : public SimpleCollisionsFixtureBase {};

    TEST_F(RaycastWithBoxTests, givenSimpleRayInsideBox_returnsIntersectionResults) {
        Ray ray(FPVector::zero(), FPVector(fp{0}, fp{1}, fp{0}));
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{3}, fp{3}, fp{3}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        EXPECT_EQ(fp{2}, timeOfIntersection);
        EXPECT_EQ(FPVector(fp{0}, fp{2}, fp{0}), pointOfIntersection);
    }

    TEST_F(RaycastWithBoxTests, givenSimpleVerticalCenteredRayInsideBox_returnsIntersectionResults) {
        FPVector center(fp{-1}, fp{-1}, fp{-1});
        Ray ray(center, FPVector(fp{0}, fp{0}, fp{1}).normalized());
        colliderA.setBox(center, FPVector(fp{3}, fp{3}, fp{4}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        TestHelpers::expectNear(fp{4}, timeOfIntersection, fp{0.01f});
        TestHelpers::expectNear(FPVector(fp{-1}, fp{-1}, fp{3}), pointOfIntersection, fp{0.1f});
    }
    
    TEST_F(RaycastWithBoxTests, givenCenteredRayInsideBox_givenRayHitsVertex_returnsIntersectionResults) {
        FPVector center(fp{-1}, fp{-1}, fp{-1});
        Ray ray(center, FPVector(fp{1}, fp{1}, fp{1}).normalized());
        colliderA.setBox(center, FPVector(fp{3}, fp{3}, fp{3}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        TestHelpers::expectNear(fp{5.196f}, timeOfIntersection, fp{0.1f});
        TestHelpers::expectNear(FPVector(fp{2}, fp{2}, fp{2}), pointOfIntersection, fp{0.1f});
    }

    TEST_F(RaycastWithBoxTests, givenCenteredRayInsideBox_givenRayHitsEdge_returnsIntersectionResults) {
        FPVector center(fp{-1}, fp{-1}, fp{-1});
        Ray ray(center, FPVector(fp{0}, fp{1}, fp{1}).normalized());
        colliderA.setBox(center, FPVector(fp{3}, fp{3}, fp{3}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        TestHelpers::expectNear(fp{4.25}, timeOfIntersection, fp{0.1f});
        TestHelpers::expectNear(FPVector(fp{-1}, fp{2}, fp{2}), pointOfIntersection, fp{0.1f});
    }

    TEST_F(RaycastWithBoxTests, givenCenteredRayInsideBox_givenRayHitsFaceNotPerpendicularly_returnsIntersectionResults) {
        FPVector center(fp{-1}, fp{-1}, fp{-1});
        Ray ray(center, FPVector(fp{0.5f}, fp{1}, fp{1}).normalized());
        colliderA.setBox(center, FPVector(fp{3}, fp{3}, fp{3}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        TestHelpers::expectNear(fp{4.5}, timeOfIntersection, fp{0.1f});
        TestHelpers::expectNear(FPVector(fp{0.5f}, fp{2}, fp{2}), pointOfIntersection, fp{0.1f});
    }

    TEST_F(RaycastWithBoxTests, givenSimpleRayOutsideBox_givenRayDoesNotIntersect_returnsNoIntersection) {
        Ray ray(FPVector(fp{2}, fp{2.1f}, fp{2}), FPVector(fp{0}, fp{1}, fp{0}));
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{3}, fp{3}, fp{3}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }

    TEST_F(RaycastWithBoxTests, givenSimpleRayOutsideBox_givenRayDoesIntersect_returnsIntersection) {
        Ray ray(FPVector(fp{1}, fp{3}, fp{1}), FPVector(fp{0}, fp{-1}, fp{0}));
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{3}, fp{3}, fp{3}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        ASSERT_TRUE(doesIntersect);
        EXPECT_EQ(fp{1}, timeOfIntersection);
        EXPECT_EQ(FPVector(fp{1}, fp{2}, fp{1}), pointOfIntersection);
    }

    TEST_F(RaycastWithBoxTests, givenRotatedBox_givenSimpleRayInBox_returnsIntersectionResults) {
        Ray ray(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{0}, fp{0}, fp{-1}));
        colliderA.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{2}, fp{2}, fp{2})
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        EXPECT_EQ(fp{2}, timeOfIntersection);
        FPVector expectedPointOfIntersection(fp{1}, fp{1}, -fp{1});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

    TEST_F(RaycastWithBoxTests, givenRayOutsideBox_givenRayDoesNotIntersect_returnsNoIntersection) {
        Ray ray(FPVector(fp{2}, fp{2}, fp{2}), FPVector(fp{0}, fp{1}, fp{0}));
        colliderA.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{1}, fp{1}, fp{1})
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }

    TEST_F(RaycastWithBoxTests, givenRayOutsideBox_givenRayDoesIntersect_returnsIntersection) {
        Ray ray(FPVector(fp{1}, fp{1}, fp{3}), FPVector(fp{0}, fp{0}, fp{-1}));
        colliderA.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{1}, fp{1}, fp{1})
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        EXPECT_EQ(fp{1}, timeOfIntersection);
        FPVector expectedPointOfIntersection(fp{1}, fp{1}, fp{2});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

    TEST_F(RaycastWithBoxTests, givenRayOnFaceOfBox_givenRayGoesInsideBox_returnsIntersection) {
        Ray ray(FPVector(fp{0}, fp{0}, fp{2}), FPVector(fp{0}, fp{0}, fp{-1}));
        colliderA.setBox(FPVector(FPVector::zero()), FPVector(fp{2}, fp{2}, fp{2}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        EXPECT_EQ(fp{0}, timeOfIntersection);
        FPVector expectedPointOfIntersection(fp{0}, fp{0}, fp{2});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

    TEST_F(RaycastWithBoxTests, givenRayOnFaceOfBox_givenRayGoesAwayFromBox_returnsNoIntersection) {
        Ray ray(FPVector(fp{0}, fp{0}, fp{2}), FPVector(fp{0}, fp{0}, fp{1}));
        colliderA.setBox(FPVector(FPVector::zero()), FPVector(fp{2}, fp{2}, fp{2}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }

    TEST_F(RaycastWithBoxTests, givenRayOutsideBox_givenRayCrossesFaceOfBoxButDoesNotGoIntoBox_returnsNoIntersection) {
        Ray ray(FPVector(fp{-4}, fp{0}, fp{2}), FPVector(fp{1}, fp{0}, fp{0}));
        colliderA.setBox(FPVector(FPVector::zero()), FPVector(fp{2}, fp{2}, fp{2}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }

    TEST_F(RaycastWithBoxTests, givenRayOutsideBox_givenRayCrossesVertexOfBoxButDoesNotGoIntoBox_returnsNoIntersection) {
        Ray ray(FPVector(fp{1}, fp{2}, fp{3}), FPVector(fp{1}, fp{0}, fp{-1}).normalized());
        colliderA.setBox(FPVector(FPVector::zero()), FPVector(fp{2}, fp{2}, fp{2}));

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.raycastWithBox(ray, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }

    // Ray crossing face but not going into box should not count
    // Ray crossing vertex but not going into box should not count
    
#pragma endregion
#pragma region Linetest vs Box

    // TODO: Unit test when ray crosses box surface. Current chosen standard is simply touching surface should NOT count as intersection

    class LinetestWithBoxTests : public SimpleCollisionsFixtureBase {};

    TEST_F(LinetestWithBoxTests, givenLineOutsideAndTowardsOBB_givenLineTooShort_returnsNoIntersection) {
        Line line(FPVector(fp{1}, fp{1}, fp{3}), FPVector(fp{1}, fp{1}, fp{2.5f}));
        colliderA.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{1}, fp{1}, fp{1})
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithBox(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }

    TEST_F(LinetestWithBoxTests, givenLineOutsideAndTowardsOBB_givenLineLongEnough_returnsIntersection) {
        Line line(FPVector(fp{1}, fp{1}, fp{3}), FPVector(fp{1}, fp{1}, fp{2}));
        colliderA.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{1}, fp{1}, fp{1})
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithBox(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        EXPECT_EQ(fp{1}, timeOfIntersection);
        FPVector expectedPointOfIntersection(fp{1}, fp{1}, fp{2});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

    TEST_F(LinetestWithBoxTests, givenLineOutsideAndAwayOBB_returnsNoIntersection) {
        Line line(FPVector(fp{1}, fp{1}, fp{3}), FPVector(fp{1}, fp{1}, fp{2000}));
        colliderA.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{1}, fp{1}, fp{1})
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithBox(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }

    TEST_F(LinetestWithBoxTests, givenLineInsideOBB_givenLineTooShort_returnsNoIntersection) {
        Line line(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{-0.5f}));
        colliderA.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{2}, fp{2}, fp{2})
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithBox(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_FALSE(doesIntersect);
    }

    TEST_F(LinetestWithBoxTests, givenLineInsideOBB_givenLineLongEnough_returnsIntersection) {
        Line line(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{-1}));
        colliderA.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{2}, fp{2}, fp{2})
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithBox(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        EXPECT_TRUE(doesIntersect);
        EXPECT_EQ(fp{2}, timeOfIntersection);
        FPVector expectedPointOfIntersection(fp{1}, fp{1}, -fp{1});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

#pragma endregion
#pragma region Linetest vs Capsule
    
    class LinetestWithCapsuleTests : public SimpleCollisionsFixtureBase {};

    TEST_F(LinetestWithCapsuleTests, givenLineIntersectsPerpendicularlyWithMedianLine_thenIntersectionReturned) {
        Line line(FPVector(fp{-10}, fp{0}, fp{0}), FPVector(fp{10}, fp{0}, fp{0}));
        colliderA.setCapsule(
            FPVector(fp{0}, fp{0}, fp{0}),
            // FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            fp{5}, fp{10}
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithCapsule(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        ASSERT_TRUE(doesIntersect);
        EXPECT_EQ(fp{0.25f}, timeOfIntersection);
        FPVector expectedPointOfIntersection(fp{-5}, fp{0}, fp{0});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

    TEST_F(LinetestWithCapsuleTests, whenTestLinePerpendicularToCap_givenLineDoesNotIntersectMedialLineButHitsTopOfSphereCap_thenCollisionReturned) {
        Line line(FPVector(fp{-10}, fp{0}, fp{10}), FPVector(fp{10}, fp{0}, fp{10}));
        colliderA.setCapsule(
            FPVector(fp{0}, fp{0}, fp{0}),
            // FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            fp{5}, fp{10}
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithCapsule(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        ASSERT_TRUE(doesIntersect);
        EXPECT_EQ(fp{0.5f}, timeOfIntersection);
        FPVector expectedPointOfIntersection(fp{0}, fp{0}, fp{10});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

    TEST_F(LinetestWithCapsuleTests, whenTestLinePerpendicularToCap_givenLineDoesNotIntersectMedialLineButHitsMidwayOfSphereCap_thenCollisionReturned) {
        Line line(FPVector(fp{-10}, fp{0}, fp{8}), FPVector(fp{10}, fp{0}, fp{8}));
        colliderA.setCapsule(
            FPVector(fp{0}, fp{0}, fp{0}),
            // FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            fp{5}, fp{10}
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithCapsule(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        ASSERT_TRUE(doesIntersect);
        TestHelpers::expectNear(fp{0.3f}, timeOfIntersection, fp{0.01f});
        FPVector expectedPointOfIntersection(fp{-4}, fp{0}, fp{8});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

    TEST_F(LinetestWithCapsuleTests, whenTestLineInsideCapsule_givenTestLineIntersectsCapsuleLineButRadiusHuge_thenIntersectionReturned) {
        Line line(FPVector(fp{-10}, fp{0}, fp{0}), FPVector(fp{10}, fp{0}, fp{0}));
        colliderA.setCapsule(
            FPVector(fp{0}, fp{0}, fp{0}),
            // FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            fp{100}, fp{1000}
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithCapsule(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        ASSERT_TRUE(doesIntersect);
        EXPECT_EQ(fp{0}, timeOfIntersection);
        FPVector expectedPointOfIntersection(fp{-10}, fp{0}, fp{0});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

    TEST_F(LinetestWithCapsuleTests, whenTestLineInsideCapsule_givenTestLineParallelCapsuleLineAndRadiusHuge_thenIntersectionReturned) {
        Line line(FPVector(fp{1}, fp{1}, fp{10}), FPVector(fp{1}, fp{1}, fp{-10}));
        colliderA.setCapsule(
            FPVector(fp{0}, fp{0}, fp{0}),
            // FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            fp{100}, fp{1000}
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithCapsule(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        ASSERT_TRUE(doesIntersect);
        EXPECT_EQ(fp{0}, timeOfIntersection);
        FPVector expectedPointOfIntersection(fp{1}, fp{1}, fp{10});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

    TEST_F(LinetestWithCapsuleTests, whenTestLineInsideCapsule_givenTestLineParallelCapsuleLineAndCrossesCapsuleExterior_thenIntersectionReturned) {
        Line line(FPVector(fp{1}, fp{1}, fp{0}), FPVector(fp{1}, fp{1}, fp{200}));
        colliderA.setCapsule(
            FPVector(fp{0}, fp{0}, fp{0}),
            // FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            fp{100}, fp{1000}
        );

        fp timeOfIntersection;
        FPVector pointOfIntersection;
        bool doesIntersect =
            simpleCollisions.linetestWithCapsule(line, colliderA, timeOfIntersection, pointOfIntersection);
        
        ASSERT_TRUE(doesIntersect);
        TestHelpers::expectNear(fp{0.5f}, timeOfIntersection, fp{0.01f});
        FPVector expectedPointOfIntersection(fp{1}, fp{1}, fp{100});
        TestHelpers::expectNear(expectedPointOfIntersection, pointOfIntersection, fp{0.01f});
    }

#pragma endregion
}