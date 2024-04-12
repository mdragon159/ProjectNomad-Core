#pragma once
#include "Math/FVectorFP.h"

struct FCollider;

namespace ProjectNomad {
    struct CoreContext;

    // Contains helper functions that SHOULD be contained within Collider but can't due to circular dependencies
    class ColliderHelpers {
      public:
        ColliderHelpers() = delete;
        
        /// <summary>
        /// Gets furthest point for the current collider in the given direction
        /// Used for determining support points in GJK/EPA algorithms
        /// Watch the following for a quick overview on supporting points for simplex: https://youtu.be/MDusDn8oTSE?t=80
        /// </summary>
        static FVectorFP GetFurthestPoint(CoreContext& coreContext, const FCollider& collider, const FVectorFP& direction);
    };
}
