#include "pchNCT.h"

#include "GameCore/CoreComponents.h"
#include "Math/FixedPoint.h"
#include "TestHelpers/TestHelpers.h"

using namespace ProjectNomad;

namespace CoreComponentsTests {
    class CoreComponentsTests : public BaseSimTest {};
    
    TEST_F(CoreComponentsTests, TransformComponent_CalculateCRC32_whenSameValues_thenChecksumAreEquivalent) {
        TransformComponent firstComp = {};
        firstComp.location = FPVector(fp{1}, fp{-100}, fp{0.5f});
        firstComp.rotation = FPQuat::identity();

        TransformComponent secondComp = {};
        secondComp.location = FPVector(fp{1}, fp{-100}, fp{0.5f});
        secondComp.rotation = FPQuat::identity();

        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_EQ(firstChecksum, secondChecksum);
    }

    TEST_F(CoreComponentsTests, TransformComponent_CalculateCRC32_whenDifferentValues_thenChecksumAreDifferent) {
        TransformComponent firstComp = {};
        firstComp.location = FPVector(fp{-0.5f}, fp{100}, fp{0});
        firstComp.rotation = FPQuat::identity();

        TransformComponent secondComp = {};
        secondComp.location = FPVector(fp{1}, fp{-100}, fp{0.5f});
        secondComp.rotation = FPQuat::identity();

        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_NE(firstChecksum, secondChecksum);
    }

    TEST_F(CoreComponentsTests, PhysicsComponent_CalculateCRC32_whenSameValues_thenChecksumAreEquivalent) {
        PhysicsComponent firstComp = {};
        firstComp.velocity = FPVector(fp{1}, fp{-100}, fp{0.5f});

        PhysicsComponent secondComp = {};
        secondComp.velocity = FPVector(fp{1}, fp{-100}, fp{0.5f});

        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_EQ(firstChecksum, secondChecksum);
    }

    TEST_F(CoreComponentsTests, PhysicsComponent_CalculateCRC32_whenDifferentValues_thenChecksumAreDifferent) {
        PhysicsComponent firstComp = {};
        firstComp.velocity = FPVector(fp{-0.5f}, fp{100}, fp{0});

        PhysicsComponent secondComp = {};
        secondComp.velocity = FPVector(fp{1}, fp{-100}, fp{0.5f});

        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_NE(firstChecksum, secondChecksum);
    }

    TEST_F(CoreComponentsTests, DynamicColliderComponent_CalculateCRC32_whenSameValues_thenChecksumAreEquivalent) {
        DynamicColliderComponent firstComp = {};
        firstComp.collider.setBox(FPVector(fp{1}, fp{-100}, fp{0.5f}), FPVector(fp{1}, fp{2}, fp{3}));

        DynamicColliderComponent secondComp = {};
        secondComp.collider.setBox(FPVector(fp{1}, fp{-100}, fp{0.5f}), FPVector(fp{1}, fp{2}, fp{3}));

        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_EQ(firstChecksum, secondChecksum);
    }

    TEST_F(CoreComponentsTests, DynamicColliderComponent_CalculateCRC32_whenDifferentValues_thenChecksumAreDifferent) {
        DynamicColliderComponent firstComp = {};
        firstComp.collider.setBox(FPVector(fp{-0.5f}, fp{100}, fp{0}), FPVector(fp{1}, fp{2}, fp{3}));

        DynamicColliderComponent secondComp = {};
        secondComp.collider.setBox(FPVector(fp{1}, fp{-100}, fp{0.5f}), FPVector(fp{1}, fp{2}, fp{3}));
        
        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_NE(firstChecksum, secondChecksum);
    }

    TEST_F(CoreComponentsTests, StaticColliderComponent_CalculateCRC32_whenSameValues_thenChecksumAreEquivalent) {
        StaticColliderComponent firstComp = {};
        firstComp.collider.setBox(FPVector(fp{1}, fp{-100}, fp{0.5f}), FPVector(fp{1}, fp{2}, fp{3}));

        StaticColliderComponent secondComp = {};
        secondComp.collider.setBox(FPVector(fp{1}, fp{-100}, fp{0.5f}), FPVector(fp{1}, fp{2}, fp{3}));

        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_EQ(firstChecksum, secondChecksum);
    }

    TEST_F(CoreComponentsTests, StaticColliderComponent_CalculateCRC32_whenDifferentValues_thenChecksumAreDifferent) {
        StaticColliderComponent firstComp = {};
        firstComp.collider.setBox(FPVector(fp{-0.5f}, fp{100}, fp{0}), FPVector(fp{1}, fp{2}, fp{3}));

        StaticColliderComponent secondComp = {};
        secondComp.collider.setBox(FPVector(fp{1}, fp{-100}, fp{0.5f}), FPVector(fp{1}, fp{2}, fp{3}));
        
        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_NE(firstChecksum, secondChecksum);
    }

    TEST_F(CoreComponentsTests, HitfreezeComponent_CalculateCRC32_whenSameValues_thenChecksumAreEquivalent) {
        HitfreezeComponent firstComp = {};
        firstComp.startingFrame = 99;
        firstComp.totalLength = 2;

        HitfreezeComponent secondComp = {};
        secondComp.startingFrame = 99;
        secondComp.totalLength = 2;
        
        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_EQ(firstChecksum, secondChecksum);
    }

    TEST_F(CoreComponentsTests, HitfreezeComponent_CalculateCRC32_whenDifferentValues_thenChecksumAreDifferent) {
        HitfreezeComponent firstComp = {};
        firstComp.startingFrame = 100;
        firstComp.totalLength = 3;

        HitfreezeComponent secondComp = {};
        secondComp.startingFrame = 99;
        secondComp.totalLength = 2;
        
        uint32_t firstChecksum = 0;
        firstComp.CalculateCRC32(firstChecksum);
        uint32_t secondChecksum = 0;
        secondComp.CalculateCRC32(secondChecksum);
        
        EXPECT_NE(firstChecksum, secondChecksum);
    }
}
