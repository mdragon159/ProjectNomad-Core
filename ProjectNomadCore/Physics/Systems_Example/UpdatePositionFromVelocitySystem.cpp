#include "UpdatePositionFromVelocitySystem.h"

#include "Helpers/PhysicsUpdateHelpers.h"
#include "Context/FrameRate.h"
#include "Context/SimContext.h"
#include "GameCore/CoreComponents.h"
#include "Utilities/Profiling.h"

namespace ProjectNomad {
    void UpdatePositionFromVelocitySystem::Update(SimContext& simContext) {
        MEASURE_SYSTEM_FUNCTION("UpdatePositionFromVelocitySystem", STAT_SYSTEM_UpdatePositionFromVelocitySystem);

        auto view = simContext.registry.view<PhysicsComponent, TransformComponent, DynamicColliderComponent>();
        for (auto &&[entityId, physicsComp, transformComp, colliderComp] : view.each()) {
            OnUpdate(simContext, entityId, physicsComp, transformComp, colliderComp);
        }
    }

    void UpdatePositionFromVelocitySystem::OnUpdate(SimContext& simContext,
                                                    entt::entity selfId,
                                                    const PhysicsComponent& physicsComp,
                                                    TransformComponent& transformComp,
                                                    DynamicColliderComponent& colliderComp) {
        // If in hitstop, then don't want any movement (for that neat juicy freeze effect)
        if (UNLIKELY(simContext.registry.any_of<HitstopComponent>(selfId))) {
            return;
        }

        FVectorFP newLocation = transformComp.location + physicsComp.velocity * FrameRate::TimePerFrameInSec();
        PhysicsUpdateHelpers::SetNewLocation(transformComp, colliderComp, newLocation);
    }
}
