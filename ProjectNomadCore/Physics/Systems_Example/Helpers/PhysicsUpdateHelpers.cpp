#include "PhysicsUpdateHelpers.h"

#include "GameCore/CoreComponents.h"
#include "Math/FVectorFP.h"

namespace ProjectNomad {
    void PhysicsUpdateHelpers::SetNewLocation(TransformComponent& transformComp,
                                              DynamicColliderComponent& colliderComp,
                                              const FVectorFP& newLocation) {
        transformComp.location = newLocation;
        colliderComp.collider.SetCenter(newLocation);
    }
}
