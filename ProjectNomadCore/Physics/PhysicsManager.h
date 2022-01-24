#pragma once

#include "Utilities/ILogger.h"
#include "ComplexCollisions.h"
#include "SimpleCollisions.h"

namespace ProjectNomad {
    template <typename LoggerType>
    class PhysicsManager {
        static_assert(std::is_base_of_v<ILogger, LoggerType>, "LoggerType must inherit from ILogger");
        
        LoggerType& logger;
        SimpleCollisions<LoggerType> simpleCollisions;
        ComplexCollisions<LoggerType> complexCollisions;
        
    public:
        PhysicsManager(LoggerType& logger)
        : logger(logger), simpleCollisions(logger), complexCollisions(logger, simpleCollisions) {}

        SimpleCollisions<LoggerType>& getSimpleCollisions() {
            return simpleCollisions;
        }
        
        ComplexCollisions<LoggerType>& getComplexCollisions() {
            return complexCollisions;
        }
    };
}
