#pragma once

struct FVectorFP;

namespace ProjectNomad {
    struct DynamicColliderComponent;
    struct TransformComponent;

    class PhysicsUpdateHelpers {
      public:
        PhysicsUpdateHelpers() = delete;

        // Sort of "reminder" that updating locating should *never* only update the transform.
        //      TODO: Would be nice to not have to update the collider manually every single time. Esp for if/when we eventually implement multi-hitboxes
        static void SetNewLocation(TransformComponent& transformComp,
                                   DynamicColliderComponent& colliderComp,
                                   const FVectorFP& newLocation);
    };
}
