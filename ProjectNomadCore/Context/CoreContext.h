#pragma once

#include "Context/SimFrame.h"
#include "EnTT/entt.hpp"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    /**
    * Context object for use by systems and the like.
    * Note that game's SimLayer project has a "SimContext". This context contains elements which the Core project contains,
    * while SimContext contains additional objects which aren't accessible in this Core project.
    **/
    struct CoreContext {
        SimFrame simFrame = {};
        entt::registry registry = {};

        // Store singleton references so not necessary to directly ::get() everywhere
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
    };
}
