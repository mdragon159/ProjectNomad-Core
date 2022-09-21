#pragma once

#include "SimFrame.h"
#include "DataProviders/MoveDataProvider.h"
#include "EnTT/entt.hpp"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    // TODO: "Circular" project dependencies,rRefactor SimContext out of Core project! Perhaps CoreContext template/base type?
    struct SimContext {
        SimFrame simFrame = {};
        entt::registry registry = {};

        // Store singleton references so not necessary to directly ::get() everywhere
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();

        // Save effort passing around objects to systems by stuffing them into the context object.
        // Note that these objects are expected to be freely used by systems. Thus, not adding certain objects which
        // need to be more careful with (eg, render interface).
        MoveDataProvider moveDataProvider;
    };
}
