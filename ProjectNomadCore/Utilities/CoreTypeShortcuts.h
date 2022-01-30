#pragma once
#include "LoggerSingleton.h"
#include "Physics/PhysicsManager.h"

namespace ProjectNomad {
    // Aliases for simplifying code
    using SimPhysicsManager = PhysicsManager<LoggerSingleton>;
}
