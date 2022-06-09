#include "pch.h"

#include "TestHelpers/TestHelpers.h"
#include "Physics/Collider.h"
#include "Physics/ComplexCollisions.h"
#include "Physics/SimpleCollisions.h"

using namespace ProjectNomad;

namespace ComplexCollisionsTests {
    // TOOD:
    // Sphere-sphere

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

#pragma region Capsule vs Sphere

#pragma region isColliding: Capsule and Sphere

    class ComplexCapsuleSphereCollisions : public ComplexCollisionsTestsBase {};

    TEST_F(ComplexCapsuleSphereCollisions, whenCenteredOverlappingColliders_statesIsColliding) {
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderA.setSphere(FPVector(fp{0}, fp{0}, fp{0}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        float x = toFloat(result.penetrationDepth.x);
        EXPECT_TRUE(result.isColliding);
    }

    TEST_F(ComplexCapsuleSphereCollisions, whenBarelyTouchingOnTopOfCapsule_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{24.9f}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
    }

    TEST_F(ComplexCapsuleSphereCollisions, whenJustTouchingOnTopOfCapsule_notColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{5}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(ComplexCapsuleSphereCollisions, whenBarelyTouchingSideOfCapsule_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{14.5f}, fp{0}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
    }

    TEST_F(ComplexCapsuleSphereCollisions, whenJustTouchingSideOfCapsule_notColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setSphere(FPVector(fp{0}, fp{15}, fp{0}), fp{5});
        
        ImpactResult result = complexCollisions.isColliding(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

#pragma endregion 
    
#pragma endregion 
    
#pragma endregion 
    
#pragma region GJK Tests
    #pragma region Wrong type checks
    class GJKWrongTypeErrors : public ComplexCollisionsTestsBase {
    protected:
        void TearDown() override {
            TestHelpers::verifyErrorsLogged(testLogger);
        }
    };

    TEST_F(GJKWrongTypeErrors, whenFirstColliderNotInitialized_thenLogsError) {
        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(GJKWrongTypeErrors, whenSecondColliderNotInitialized_thenLogsError) {
        colliderA.setSphere(FPVector::zero(), fp{5});

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_FALSE(result);
    }
    
    #pragma endregion
    #pragma region isColliding: Box and Box

    class GJKBoxBox : public ComplexCollisionsTestsBase {};

    TEST_F(GJKBoxBox, whenBoxesAreDistant_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}));

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(GJKBoxBox, whenTouching_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(GJKBoxBox, whenIntersectingBarely_returnsCollision) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(GJKBoxBox, whenIntersectingSignificantly_returnsCollision) {
        colliderA.setBox(FPVector(fp{-0.5f}, fp{-0.5f}, fp{-0.5f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    TEST_F(GJKBoxBox, whenRotatedOBBs_whenNotIntersecting_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}),
                         FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
                         FPVector(fp{1}, fp{1}, fp{1})
        );

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(GJKBoxBox, whenRotatedOBBs_whenIntersecting_returnsCollision) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}),
                         FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
                         FPVector(fp{1}, fp{1}, fp{1})
        );

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    #pragma endregion
    #pragma region isColliding: Capsule and Capsule

    class GJKCapsuleCapsule : public ComplexCollisionsTestsBase {};
    
    TEST_F(GJKCapsuleCapsule, whenIdenticalOverlappingColliders_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    // TODO tests. (Can try to copy SimpleCollisions, but may be a waste when switch to GJK)
    // identical overlapping capsules
    // rotated identically sized capsules with some displacement
    // touching capsules
    // nowhere close capsules
    // (perhaps look at implementation and create additional tests)

    #pragma endregion
    #pragma region isColliding: Sphere and Sphere

    class GJKSphereSphere : public ComplexCollisionsTestsBase {};

    TEST_F(GJKSphereSphere, whenDistantSpheres_statesNotColliding) {
        colliderA.setSphere(FPVector(fp{-20}, fp{5}, fp{10}), fp{5});
        colliderB.setSphere(FPVector(fp{20}, fp{0.5f}, fp{0}), fp{1});

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(GJKSphereSphere, whenSpheresTouching_statesNotColliding) {
        colliderA.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        colliderB.setSphere(FPVector(fp{5}, fp{0}, fp{0}), fp{5});

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(GJKSphereSphere, whenSpheresIntersecting_statesIsColliding) {
        colliderA.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        colliderB.setSphere(FPVector(fp{5}, fp{0}, fp{0}), fp{6});

        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_TRUE(result);

    }

    #pragma endregion
    #pragma region isColliding: Box and Capsule

    class GJKBoxCapsule : public ComplexCollisionsTestsBase {};

    TEST_F(GJKBoxCapsule, whenDistantColliders_whenNoRotation_statesIsColliding) {
        // Case where median line is inside expanded box (box half size increased by radius)
        colliderA.setBox(FPVector(fp{-20}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_FALSE(result);
    }

    TEST_F(GJKBoxCapsule, whenCollidersOverlappingAlongCapsuleTop_whenNoRotation_statesIsColliding) {
        // Case where median line is inside expanded box (box half size increased by radius)
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{20}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_TRUE(result);
    }
    
    TEST_F(GJKBoxCapsule, whenCenteredOverlappingColliders_whenMedialLineWithinExpandedBox_statesIsColliding) {
        // Case where median line is inside expanded box (box half size increased by radius)
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
        EXPECT_TRUE(result);
    }

    // # All with rotations and without rotations
    // Not touching (no rotations) = no intersection
    // Touching = no intersection
    // Intersecting a little = intersection
    // Capsule entirely inside box = intersection

    // Then look at implementation and hit different noted cases

    #pragma endregion
    #pragma region isColliding: Box and Sphere

     class GJKBoxSphere : public ComplexCollisionsTestsBase {};

     TEST_F(GJKBoxSphere, givenNoRotationBox_givenSphereDistantFromBox_statesNotColliding) {
         colliderB.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{4}, fp{4}, fp{4}));
         colliderA.setSphere(FPVector(fp{-5}, fp{5}, fp{10}), fp{5});
         
         bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
         EXPECT_FALSE(result);
     }

     TEST_F(GJKBoxSphere, givenNoRotationBox_givenSphereTouchingBox_statesColliding) {
         colliderA.setBox(FPVector(fp{1}, fp{0}, fp{0}), FPVector(fp{1}, fp{1}, fp{1}));
         colliderB.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
         
         bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
         EXPECT_TRUE(result);
     }

     TEST_F(GJKBoxSphere, givenNoRotationBox_givenSphereIntersectingBox_statesIsColliding) {
         colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{1}, fp{1}, fp{1}));
         colliderB.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
         
         bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
         EXPECT_TRUE(result);
     }

     TEST_F(GJKBoxSphere, givenNoRotationBox_givenSphereCenteredWithinBox_statesIsColliding) {
         colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{2}, fp{2}, fp{2}));
         colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{0}), fp{1});
         
         bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
         EXPECT_TRUE(result);
     }

     TEST_F(GJKBoxSphere, givenNoRotationBox_givenSphereWithinBoxButNotCentered_statesIsColliding) {
         colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{3}, fp{3}, fp{3}));
         colliderB.setSphere(FPVector(fp{1}, fp{1}, fp{1}), fp{1});
         
         bool result = complexCollisions.isCollidingViaGJK(colliderA, colliderB);
         EXPECT_TRUE(result);
     }

    #pragma endregion
    #pragma region isColliding: Capsule and Sphere

    // # All with rotations and without rotations
    // Not touching (no rotations) = no intersection
    // Touching = no intersection
    // Intersecting a little = intersection
    // Capsule entirely inside box = intersection

    // Then look at implementation and hit different noted cases
    
    #pragma endregion
#pragma endregion

#pragma region computeComplexCollision Tests
    #pragma region Wrong type checks
    class EPAWrongTypeErrors : public ComplexCollisionsTestsBase {
    protected:
        Collider colliderA;
        Collider colliderB;
        
        void TearDown() override {
            TestHelpers::verifyErrorsLogged(testLogger);
        }
    };

    TEST_F(EPAWrongTypeErrors, whenFirstColliderNotInitialized_thenLogsError) {
        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(EPAWrongTypeErrors, whenSecondColliderNotInitialized_thenLogsError) {
        colliderA.setSphere(FPVector::zero(), fp{5});

        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }
    
    #pragma endregion
    #pragma region isColliding: Box and Box

    class EPABoxBox : public ComplexCollisionsTestsBase {};

    TEST_F(EPABoxBox, whenBoxesAreDistant_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}));

        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(EPABoxBox, whenTouching_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(EPABoxBox, whenIntersecting_returnsCollisionWithPenDepth) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}), FPVector(fp{1}, fp{1}, fp{1}));

        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(FPVector(fp{0.1f}, fp{0}, fp{0}), result.penetrationDepth, fp{0.1f});
    }

    TEST_F(EPABoxBox, whenRotatedOBBs_whenNotIntersecting_statesNotColliding) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{1}, fp{1}, fp{1}),
                         FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
                         FPVector(fp{1}, fp{1}, fp{1})
        );

        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(EPABoxBox, whenRotatedOBBs_whenIntersecting_returnsCollisionWithPenDepth) {
        colliderA.setBox(FPVector(fp{-0.9f}, fp{-0.9f}, fp{-0.9f}), FPVector(fp{1}, fp{1}, fp{1}));
        colliderB.setBox(FPVector(fp{0.5f}, fp{0.5f}, fp{0.5f}),
                         FPQuat::fromDegrees(FPVector(fp{0}, fp{0}, fp{1}), fp{45}),
                         FPVector(fp{1}, fp{1}, fp{1})
        );

        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(FPVector(fp{0.3f}, fp{0.3f}, fp{0}), result.penetrationDepth, fp{0.01f});
    }

    #pragma endregion
    #pragma region isColliding: Capsule and Capsule

    class EPACapsuleCapsule : public ComplexCollisionsTestsBase {};
    
    TEST_F(EPACapsuleCapsule, whenIdenticalOverlappingColliders_statesIsColliding) {
        colliderA.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
    }

    // TODO tests. (Can try to copy SimpleCollisions, but may be a waste when switch to GJK)
    // identical overlapping capsules
    // rotated identically sized capsules with some displacement
    // touching capsules
    // nowhere close capsules
    // (perhaps look at implementation and create additional tests)

    #pragma endregion
    #pragma region isColliding: Sphere and Sphere

    class EPASphereSphere : public ComplexCollisionsTestsBase {};

    TEST_F(EPASphereSphere, whenDistantSpheres_statesNotColliding) {
        colliderA.setSphere(FPVector(fp{-20}, fp{5}, fp{10}), fp{5});
        colliderB.setSphere(FPVector(fp{20}, fp{0.5f}, fp{0}), fp{1});

        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(EPASphereSphere, whenSpheresTouching_statesNotColliding) {
        colliderA.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        colliderB.setSphere(FPVector(fp{5}, fp{0}, fp{0}), fp{5});

        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_FALSE(result.isColliding);
    }

    TEST_F(EPASphereSphere, whenSpheresIntersecting_statesIsColliding) {
        colliderA.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
        colliderB.setSphere(FPVector(fp{5}, fp{0}, fp{0}), fp{6});

        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
        TestHelpers::expectNear(FPVector(fp{1}, fp{0}, fp{0}), result.penetrationDepth, fp{0.01f});

    }

    #pragma endregion
    #pragma region isColliding: Box and Capsule

    class EPABoxCapsule : public ComplexCollisionsTestsBase {};

    TEST_F(EPABoxCapsule, whenCenteredOverlappingColliders_whenMedialLineWithinExpandedBox_statesIsColliding) {
        // Case where median line is inside expanded box (box half size increased by radius)
        colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{4}, fp{4}, fp{4}));
        colliderB.setCapsule(FPVector(fp{0}, fp{0}, fp{0}), fp{10}, fp{20});
        
        ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
        EXPECT_TRUE(result.isColliding);
    }

    // # All with rotations and without rotations
    // Not touching (no rotations) = no intersection
    // Touching = no intersection
    // Intersecting a little = intersection
    // Capsule entirely inside box = intersection

    // Then look at implementation and hit different noted cases

    #pragma endregion
    #pragma region isColliding: Box and Sphere

     class EPABoxSphere : public ComplexCollisionsTestsBase {};

     TEST_F(EPABoxSphere, givenNoRotationBox_givenSphereDistantFromBox_statesNotColliding) {
         colliderB.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{4}, fp{4}, fp{4}));
         colliderA.setSphere(FPVector(fp{-5}, fp{5}, fp{10}), fp{5});
         
         ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
         EXPECT_FALSE(result.isColliding);
     }

     TEST_F(EPABoxSphere, givenNoRotationBox_givenSphereTouchingBox_statesNotColliding) {
         colliderA.setBox(FPVector(fp{1}, fp{0}, fp{0}), FPVector(fp{1}, fp{1}, fp{1}));
         colliderB.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
         
         ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
         EXPECT_FALSE(result.isColliding);
     }

     TEST_F(EPABoxSphere, givenNoRotationBox_givenSphereIntersectingBox_statesIsColliding) {
         colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{1}, fp{1}, fp{1}));
         colliderB.setSphere(FPVector(fp{-5}, fp{0}, fp{0}), fp{5});
         
         ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
         EXPECT_TRUE(result.isColliding);
         TestHelpers::expectNear(FPVector(fp{-1}, fp{0}, fp{0}), result.penetrationDepth, fp{0.01f});
     }

     TEST_F(EPABoxSphere, givenNoRotationBox_givenSphereCenteredWithinBox_statesIsColliding) {
         colliderA.setBox(FPVector(fp{0}, fp{0}, fp{0}), FPVector(fp{2}, fp{2}, fp{2}));
         colliderB.setSphere(FPVector(fp{0}, fp{0}, fp{0}), fp{1});
         
         ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
         EXPECT_TRUE(result.isColliding);

         // TODO: Currently not calculating penetration depth for within case, must do so in future!
         EXPECT_EQ(FPVector::zero(), result.penetrationDepth);
     }

     TEST_F(EPABoxSphere, givenNoRotationBox_givenSphereWithinBoxButNotCentered_statesIsColliding) {
         colliderA.setBox(FPVector(fp{-1}, fp{-1}, fp{-1}), FPVector(fp{3}, fp{3}, fp{3}));
         colliderB.setSphere(FPVector(fp{1}, fp{1}, fp{1}), fp{1});
         
         ImpactResult result = complexCollisions.computeComplexCollision(colliderA, colliderB);
         EXPECT_TRUE(result.isColliding);

         // TODO: Currently not calculating penetration depth for within case, must do so in future!
         EXPECT_EQ(FPVector::zero(), result.penetrationDepth);
     }

    #pragma endregion
    #pragma region isColliding: Capsule and Sphere

    // # All with rotations and without rotations
    // Not touching (no rotations) = no intersection
    // Touching = no intersection
    // Intersecting a little = intersection
    // Capsule entirely inside box = intersection

    // Then look at implementation and hit different noted cases
    
    #pragma endregion 
#pragma endregion
    
}
