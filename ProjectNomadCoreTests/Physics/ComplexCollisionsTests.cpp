#include "pchNCT.h"

#include "TestHelpers/TestHelpers.h"
#include "Physics/Collider.h"
#include "Physics/ComplexCollisions.h"
#include "Physics/SimpleCollisions.h"

using namespace ProjectNomad;

namespace ComplexCollisionsTests {
    class ComplexCollisionsTestsBase : public BaseSimTest {
    protected:
        SimpleCollisions<TestLogger> simpleCollisions;
        ComplexCollisions<TestLogger> complexCollisions;
        
        Collider colliderA;
        Collider colliderB;

        ComplexCollisionsTestsBase()
        : BaseSimTest(), simpleCollisions(testLogger), complexCollisions(testLogger, simpleCollisions) {}
    };

#pragma region Direct Collision Tests

#pragma region isColliding: Box and Box

    class ComplexBoxBoxCollisions : public ComplexCollisionsTestsBase {};

    TEST_F(ComplexBoxBoxCollisions, whenBoxesAreDistant_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}));
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
        // TODO: Penetration depth x axis verification (esp since relying on direction being correct)
    }

    TEST_F(ComplexBoxBoxCollisions, whenTouching_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(ComplexBoxBoxCollisions, whenIntersecting_statesIsColliding) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
    }

    TEST_F(ComplexBoxBoxCollisions, whenRotatedOBBs_whenNotIntersecting_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(
            FPVector(fp{1}, fp{1}, fp{1}),
            FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{1}, fp{1}, fp{1})
        );
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(ComplexBoxBoxCollisions, whenRotatedOBBs_whenIntersecting_statesIsColliding) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(
        FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}),
        FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
            FPVector(fp{1}, fp{1}, fp{1})
        );
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
    }
    
#pragma endregion
#pragma region isColliding: Capsule and Capsule

    class ComplexCapsuleCapsuleCollisions : public ComplexCollisionsTestsBase {};

    TEST_F(ComplexCapsuleCapsuleCollisions, whenIdenticalOverlappingColliders_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(fp{-20}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::backward(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexCapsuleCapsuleCollisions, whenBarelyCollidingOnEnds_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{39.9f}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(fp{-0.1f}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::up(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexCapsuleCapsuleCollisions, whenJustTouchingOnEnds_notColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{40}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(ComplexCapsuleCapsuleCollisions, whenNowhereClose_notColliding) {
        colliderA.setCapsule(FPVector(fp{100}, fp{-30}, fp{40}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{-5}, fp{20.5f}, fp{100}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
        // TODO: Fix unit test (and corresponding test in SimpleCollisionsTests)
    }

    TEST_F(ComplexCapsuleCapsuleCollisions, whenIdenticalSizedOverlappingColliders_withSingleRotation_statesIsColliding) {
        FPQuat rotationAroundX = FPQuat::fromDegrees(FPVector(fp{1}, fp{0}, fp{0}), fp{90});
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), rotationAroundX, fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(fp{-20.f}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::forward(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexCapsuleCapsuleCollisions, whenBarelyCollidingOnOneEnd_withRotations_statesIsColliding) {
        FPQuat rotationAroundX = FPQuat::fromDegrees(FPVector(fp{1}, fp{0}, fp{0}), fp{90});
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{29.9f}), rotationAroundX, fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(fp{-0.1f}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::up(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexCapsuleCapsuleCollisions, whenJustTouchingOnOneEnd_withRotations_notColliding) {
        FPQuat rotationAroundX = FPQuat::fromDegrees(FPVector(fp{1}, fp{0}, fp{0}), fp{90});
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{30}), rotationAroundX, fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
        // TODO: Fix this test and the SimpleCollisions one too
    }

    TEST_F(ComplexCapsuleCapsuleCollisions, whenNowhereClose_withRotations_notColliding) {
        FPQuat rotationAroundX = FPQuat::fromDegrees(FPVector(fp{1}, fp{0}, fp{0}), fp{90});
        colliderA.setCapsule(FPVector(fp{100}, fp{-30}, fp{40}), rotationAroundX, fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{-5}, fp{20.5f}, fp{100}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }
    
#pragma endregion
#pragma region isColliding: Box and Capsule

    // TODO: Test rotations!

    class ComplexBoxCapsuleCollisions : public ComplexCollisionsTestsBase {};

    TEST_F(ComplexBoxCapsuleCollisions, whenCenteredOverlappingColliders_whenMedialLineWithinExpandedBox_statesIsColliding) {
        // Case where median line is inside expanded box (box half size increased by radius)
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result.isColliding);

        TestHelpers::expectNear(fp{14}, result.penetrationMagnitude, fp{0.01f});
        // Note that direction can be anything that's on xy plane
        TestHelpers::expectNear(result.penetrationDirection.getLength(), fp{1}, fp{0.01f});
        bool isPenDirPerpendicularToVertical = FPMath::isNear(
            result.penetrationDirection.dot(FPVector::up()), fp{0}, fp{0.01f}
        );
        EXPECT_TRUE(isPenDirPerpendicularToVertical);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenCenteredOverlappingColliders_whenMedianLineCrossesExpandedBox_statesIsColliding) {
        colliderB.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{100});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result.isColliding);

        TestHelpers::expectNear(fp{14}, result.penetrationMagnitude, fp{0.01f});
        // Note that direction can be anything that's on xy plane
        TestHelpers::expectNear(result.penetrationDirection.getLength(), fp{1}, fp{0.01f});
        bool isPenDirPerpendicularToVertical = FPMath::isNear(
            result.penetrationDirection.dot(FPVector::up()), fp{0}, fp{0.01f}
        );
        EXPECT_TRUE(isPenDirPerpendicularToVertical);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenCenteredOverlappingColliders_whenCapsuleEntirelyWithinLargeBox_statesIsColliding) {
        colliderB.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{50}, fp{50}, fp{50}));
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{5}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result.isColliding);

        TestHelpers::expectNear(fp{55}, result.penetrationMagnitude, fp{0.01f});
        // Note that direction can be anything that's on xy plane
        TestHelpers::expectNear(result.penetrationDirection.getLength(), fp{1}, fp{0.01f});
        bool isPenDirPerpendicularToVertical = FPMath::isNear(
            result.penetrationDirection.dot(FPVector::up()), fp{0}, fp{0.01f}
        );
        EXPECT_TRUE(isPenDirPerpendicularToVertical);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenSomewhatIntersectingOnOneEndOfCapsule_whenCapsuleAbove_statesIsColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{18}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        
        ASSERT_TRUE(result.isColliding);
        TestHelpers::expectNear(fp{6}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::up(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenBarelyIntersectingOnOneEndOfCapsule_whenCapsuleBelow_statesIsColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{-23.9f}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        
        ASSERT_TRUE(result.isColliding);
        TestHelpers::expectNear(fp{0.1f}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::down(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenTouchingOnOneEndOfCapsule_fromAbove_notColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{24}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_FALSE(result.isColliding);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenTouchingOnOneEndOfCapsule_fromBelow_notColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{-24}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_FALSE(result.isColliding);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenTouchingOnSideOfCapsule_fromBack_notColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{-14}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_FALSE(result.isColliding);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenTouchingOnSideOfCapsule_fromFront_notColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{14}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_FALSE(result.isColliding);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenBoxToSideOfCapsuleEndSphere_givenBoxOutsideSphereButWithinExtendedBox_notColliding) {
        // Set up case from manual testing in interactable tool where box is clearly not touching top of capsule's sphere,
        //  but would be touching a box which covers the capsule (ie, not treating ends of capsule as spheres)
        colliderA.setCapsule(FPVector(fp{120}, fp{-4}, fp{182}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{120}, fp{-77}, fp{281}), FPVector(fp{50}));
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_FALSE(result.isColliding);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenBoxToSideOfCapsule_givenCapsuleMedianLineNotInsideBoxButColliding_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{120}, fp{-4}, fp{182}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{120}, fp{52}, fp{204}), FPVector(fp{50}));
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result.isColliding);
        
        TestHelpers::expectNear(fp{19}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::right(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenTinyBoxToSideOfCapsule_givenCapsuleMedianLineNotInsideBoxButColliding_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{120}, fp{-4}, fp{182}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{120}, fp{21}, fp{184}), FPVector(fp{5}));
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result.isColliding);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenLargeBoxTouchingTopHemisphereOfCapsule_givenBoxNotTouchingMedianLine_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{120}, fp{-4}, fp{182}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{120}, fp{50}, fp{262}), FPVector(fp{50}));
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result.isColliding);
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenLargeBoxTouchingRightSideOfCapsule_givenNotTouchingMedialLineAndCapsuleHigherThanRadius_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{-51}, fp{26}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{50}));
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result.isColliding);
        
        TestHelpers::expectNear(fp{24}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::right(), result.penetrationDirection, fp{0.01f});
    }
    
    TEST_F(ComplexBoxCapsuleCollisions, whenCapsuleTouchingFrontSurfaceOfLargeBox_statesNotColliding) {
        // This test has the median line running parallel to box face and only touching the face,
        //      which we've chosen to not consider as a collision
        colliderA.setCapsule(FPVector(fp{-75}, fp{-8}, fp{54}), fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{50}));
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_FALSE(result.isColliding);
    }
    
    TEST_F(ComplexBoxCapsuleCollisions, whenRotatedCapsuleIntersectsBoxCorner_statesIsCollidingWithProperPenInfo) {
        // Direction is correct but penetration magnitude is quite off (~5x)
        FPQuat capsuleRotation = FPQuat::fromDegrees(FPVector::left(), fp{30});
        colliderA.setCapsule(FPVector(fp{75}, fp{8}, fp{54}), capsuleRotation, fp{25}, fp{50});
        colliderB.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{50}));

        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result.isColliding);
        
        TestHelpers::expectNear(fp{1}, result.penetrationMagnitude, fp{0.01f});

        // Too lazy to do the math on this side atm
        // FPQuat expectedPenetrationDir = FPQuat::fromDegrees(FPVector::left(), fp{30});
        // TestHelpers::expectNear(FPVector::right(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenCapsulePenetratingUpsideDownBox_thenPenetrationDirectionIsNotReversed) {
        colliderA.setCapsule(FPVector(fp{0}, fp{20}, fp{150}), fp{25}, fp{50});
        colliderB.setBox(
            FPVector(fp{0}, fp{0}, fp{0}),
            FPQuat::fromDegrees(FPVector::forward(), fp{180}),
            FPVector(fp{100}, fp{100}, fp{200})
        );

        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_TRUE(result.isColliding);
        
        TestHelpers::expectNear(fp{100}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::down(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexBoxCapsuleCollisions, whenDistant_notColliding) {
        colliderA.setBox(FPVector(fp{200}, fp{33.3f}, fp{-5}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{5}, fp{-10}, fp{24}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        ASSERT_FALSE(result.isColliding);
    }

#pragma endregion
#pragma region isColliding: Box and Sphere

    class ComplexBoxSphereCollisions : public ComplexCollisionsTestsBase {};

    TEST_F(ComplexBoxSphereCollisions, givenNoRotationBox_givenSphereDistantFromBox_statesNotColliding) {
        colliderB.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderA.setSphere(FPVector(fp{-5}, fp{5}, fp{10}), fp{5});
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
        // TODO: Fix test! SimpleCollisions doesn't fail!
    }

    TEST_F(ComplexBoxSphereCollisions, givenNoRotationBox_givenSphereTouchingBox_statesNotColliding) {
        colliderA.setBox(FPVector(fp{1}, fp{0}, fp{0}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(ComplexBoxSphereCollisions, givenNoRotationBox_givenSphereIntersectingBox_statesIsColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
    }

    TEST_F(ComplexBoxSphereCollisions, givenNoRotationBox_givenSphereCenteredWithinBox_statesIsColliding) {
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{2}, fp{2}, fp{2}));
        colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{0}), fp{1});
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
    }

    TEST_F(ComplexBoxSphereCollisions, givenNoRotationBox_givenSphereWithinBoxButNotCentered_statesIsColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{3}, fp{3}, fp{3}));
        colliderB.setSphere(FPVector(fp{1}, fp{1}, fp{1}), fp{1});
        
        auto result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
    }

#pragma endregion
#pragma region isColliding: Capsule and Sphere

    class ComplexCapsuleSphereCollisions : public ComplexCollisionsTestsBase {};

    TEST_F(ComplexCapsuleSphereCollisions, whenCenteredOverlappingColliders_statesIsColliding) {
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderA.setSphere(FPVector(fp{0}, fp{0}, fp{0}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(fp{-15}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::backward(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexCapsuleSphereCollisions, whenBarelyTouchingOnTopOfCapsule_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{24.9f}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(fp{-0.1f}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::down(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexCapsuleSphereCollisions, whenJustTouchingOnTopOfCapsule_notColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{5}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
        // TODO: Test is failing atm, need to fix (here and for SimpleCollisionsTests)
    }

    TEST_F(ComplexCapsuleSphereCollisions, whenBarelyTouchingSideOfCapsule_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{14.5f}, fp{0}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(fp{-0.5f}, result.penetrationMagnitude, fp{0.01f});
        TestHelpers::expectNear(FPVector::left(), result.penetrationDirection, fp{0.01f});
    }

    TEST_F(ComplexCapsuleSphereCollisions, whenJustTouchingSideOfCapsule_notColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{15}, fp{0}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

#pragma endregion

#pragma endregion 
    
}
