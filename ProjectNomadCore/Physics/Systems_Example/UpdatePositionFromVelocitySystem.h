#pragma once

#include <EnTT/entt.hpp>

namespace ProjectNomad {
    struct TransformComponent;
    struct SimContext;
    struct DynamicColliderComponent;
    struct PhysicsComponent;

    class UpdatePositionFromVelocitySystem {
      public:
        UpdatePositionFromVelocitySystem() = delete;

        static void Update(SimContext& simContext);

      private:
        static void OnUpdate(SimContext& simContext,
                             entt::entity selfId,
                             const PhysicsComponent& physicsComp,
                             TransformComponent& transformComp,
                             DynamicColliderComponent& colliderComp);
    };
}
