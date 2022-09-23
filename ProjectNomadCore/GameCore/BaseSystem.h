#pragma once
#include "Context/SimContext.h"

namespace ProjectNomad {
    /// <summary>
    /// All EnTT systems are expected to extend from this class for simplicity in usage.
    /// However, would greatly prefer avoiding virtual functions for performance and thus will
    /// likely use an alternative approach in the future
    /// </summary>
    class BaseSystem {
    public:
        virtual ~BaseSystem() {}

        virtual void Update(CoreContext& coreContext) {}
    };
}
