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
        
        virtual void logInfoMessage(const std::string& identifier, const std::string& infoMessage) = 0;

        virtual void logWarnMessage(const std::string& identifier, const std::string& warningMessage) = 0;
        
        virtual void logErrorMessage(const std::string& identifier, const std::string& errorMessage) = 0;

        virtual void addDebugMessage(DebugMessage debugMessage) = 0;

        virtual void addLogMessage(const std::string& message) = 0;

        virtual void addScreenAndLogMessage(fp displayLength, const std::string& message) = 0;

        virtual void addScreenAndLogMessage(fp displayLength, const std::string& message, LogSeverity logSeverity, OutputColor outputColor) = 0;

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
        
        virtual void addNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color) = 0;

        virtual void addNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color, NetLogCategory category) = 0;

        virtual void addInfoNetLog(const std::string& identifier, const std::string& message) = 0;

        virtual void addInfoNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) = 0;
        
        virtual void addWarnNetLog(const std::string& identifier, const std::string& message) = 0;

        virtual void addWarnNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) = 0;

        virtual void addErrorNetLog(const std::string& identifier, const std::string& message) = 0;

        virtual void addErrorNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) = 0;

        #pragma endregion 
    };
}
