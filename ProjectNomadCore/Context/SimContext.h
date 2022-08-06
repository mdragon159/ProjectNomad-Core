#pragma once
#include "SimFrame.h"
#include "EnTT/entt.hpp"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    struct SimContext {
        SimFrame simFrame = {};
        entt::registry registry = {};

        // References so don't necessary need to directly get these everywhere
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
    };
}
