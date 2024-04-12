#pragma once

#include <EnTT/entt.hpp>

#include "GameCore/CoreComponents.h"
#include "Math/FVectorFP.h"

struct FCollider;

namespace ProjectNomad {
    struct TransformComponent;
    struct SimContext;
    struct DynamicColliderComponent;
    struct PhysicsComponent;

    class HandleDynamicVsStaticCollisions {
      public:
        HandleDynamicVsStaticCollisions() = delete;

        // This method does not set the intended position (albeit it tries to remove velocity in collision direction).
        // Instead, this method takes the input position then tries to resolve any static world collisions while
        //      adjusting that position accordingly (until no collisions found OR collision pass limit reached).
        static void ProcessStaticCollisionsUntilLimitReached(SimContext& simContext,
                                                             entt::entity selfId,
                                                             const DynamicColliderComponent& colliderComp,
                                                             PhysicsComponent& physicsComp,
                                                             FVectorFP& newIntendedPos,
                                                             bool& wasMaxCollisionPassesReached);
        
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
                                                               const entt::entity& selfId,
                                                               const DynamicColliderComponent& colliderComp,
                                                               PhysicsComponent& physicsComp,
                                                               FVectorFP& newIntendedPos);

        /**
         * Checks for and resolves a single collision
         * @return true if any collision found
         */
        static bool CheckAndResolveIndividualCollision(SimContext& simContext,
                                                       FCollider& futureCollider,
                                                       const FCollider& checkAgainstCollider,
                                                       PhysicsComponent& physicsComp);
    };
}
