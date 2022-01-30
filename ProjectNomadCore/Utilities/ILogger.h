#pragma once
#include <queue>

#include "NetLogMessage.h"
#include "Utilities/DebugMessage.h"
#include "Physics/Collider.h"

namespace ProjectNomad {
    class ILogger {
    public:
        virtual ~ILogger() {}
        
        #pragma region General Debug Messages

        virtual std::queue<DebugMessage>& getDebugMessages() = 0;
        
        virtual void logInfoMessage(std::string identifier, std::string infoMessage) = 0;

        virtual void logWarnMessage(std::string identifier, std::string warningMessage) = 0;
        
        virtual void logErrorMessage(std::string identifier, std::string errorMessage) = 0;

        virtual void addDebugMessage(DebugMessage debugMessage) = 0;

        virtual void addLogMessage(std::string message) = 0;

        virtual void addScreenAndLogMessage(fp displayLength, std::string message) = 0;

        virtual void addScreenAndLogMessage(fp displayLength, std::string message, OutputColor outputColor) = 0;

        virtual void addShapeMessage(fp displayTime, const Collider& collider) = 0;

        virtual void addShapeMessage(fp displayTime, const Collider& collider, OutputColor outputColor) = 0;

        virtual void addBoxMessage(fp displayTime, const Collider& box) = 0;

        virtual void addBoxMessage(fp displayTime, const Collider& box, OutputColor outputColor) = 0;

        virtual void addSphereMessage(fp displayTime, const Collider& sphere) = 0;

        virtual void addSphereMessage(fp displayTime, const Collider& sphere, OutputColor outputColor) = 0;

        virtual void addCapsuleMessage(fp displayTime, const Collider& capsule) = 0;

        virtual void addCapsuleMessage(fp displayTime, const Collider& capsule, OutputColor outputColor) = 0;

#pragma endregion

        #pragma region NetLog Messages

        virtual std::queue<NetLogMessage>& getNetLogMessages() = 0;
        
        virtual void addNetLogMessage(std::string message) = 0;

        virtual void addNetLogMessage(std::string message, NetLogCategory category) = 0;

        virtual void addNetLogMessage(std::string message, OutputColor color) = 0;

        virtual void addNetLogMessage(std::string message, OutputColor color, NetLogCategory category) = 0;

        virtual void addInfoNetLog(std::string identifier, std::string message) = 0;

        virtual void addInfoNetLog(std::string identifier, std::string message, NetLogCategory category) = 0;
        
        virtual void addWarnNetLog(std::string identifier, std::string message) = 0;

        virtual void addWarnNetLog(std::string identifier, std::string message, NetLogCategory category) = 0;

        virtual void addErrorNetLog(std::string identifier, std::string message) = 0;

        virtual void addErrorNetLog(std::string identifier, std::string message, NetLogCategory category) = 0;

        #pragma endregion 
    };
}
