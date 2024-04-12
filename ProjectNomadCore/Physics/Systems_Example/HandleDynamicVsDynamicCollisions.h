#pragma once

#include <EnTT/entt.hpp>

#include "GameCore/CoreComponents.h"
#include "Math/FVectorFP.h"

namespace ProjectNomad {
    struct ImpactResult;
}

struct FCollider;

namespace ProjectNomad {
    struct TransformComponent;
    struct SimContext;
    struct DynamicColliderComponent;
    struct PhysicsComponent;

    class HandleDynamicVsDynamicCollisions {
      public:
        HandleDynamicVsDynamicCollisions() = delete;
        
        static void TryMoveToLocationAndProcessDynamicCollisions(SimContext& simContext,
                                                             entt::entity selfId,
                                                             TransformComponent& transformComp,
                                                             DynamicColliderComponent& colliderComp,
                                                             PhysicsComponent& physicsComp,
                                                             const FVectorFP& newIntendedPos);
        
        static void Update(SimContext& simContext);

      private:
        static void OnUpdate(SimContext& simContext,
                             entt::entity selfId,
                             PhysicsComponent& physicsComp,
                             TransformComponent& transformComp,
                             DynamicColliderComponent& colliderComp);
        
        /**
         * Checks for and resolves collisions against entire (static) world.
         * @return true if any collision found
         */
        static bool DoSinglePassCollisionCheckingAndResolution(SimContext& simContext,
                                                               entt::entity movingEntityId,
                                                               TransformComponent& movingTransformComp,
                                                               DynamicColliderComponent& movingColliderComp,
                                                               PhysicsComponent& movingPhysicsComp);

        /**
         * Checks for and resolves a single collision
         * @return true if any collision found
         */
        static bool CheckAndResolveIndividualCollision(SimContext& simContext,
                                                       TransformComponent& firstTransformComp,
                                                       DynamicColliderComponent& firstColliderComp,
                                                       PhysicsComponent& firstPhysicsComp,
                                                       TransformComponent& secondTransformComp,
                                                       DynamicColliderComponent& secondColliderComp,
                                                       PhysicsComponent& secondPhysicsComp);

        static void ResolveCollisionBetweenDifferentWeights(SimContext& simContext,
                                                            fp massRatioWithHeavierObjectAsNumerator,
                                                            TransformComponent& heavierObjectTransform,
                                                            DynamicColliderComponent& heavierObjectCollider,
                                                            PhysicsComponent& heavierObjectPhysics,
                                                            const ImpactResult& impactInfoFromLighterObjectPerspective,
                                                            TransformComponent& lighterObjectTransform,
                                                            DynamicColliderComponent& lighterObjectCollider,
                                                            PhysicsComponent& lighterObjectPhysics);
        static void ApplyCollisionResolution(const FVectorFP& penetrationDirection,
                                             fp penetrationMagnitude,
                                             fp collisionDirVelocityReductionPercentage,
                                             TransformComponent& transformComp,
                                             DynamicColliderComponent& colliderComp,
                                             PhysicsComponent& physicsComp);
    };
}
