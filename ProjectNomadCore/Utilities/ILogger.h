#pragma once
#include <queue>

#include "NetLogMessage.h"
#include "Utilities/DebugMessage.h"
#include "Physics/Model/FCollider.h"

namespace ProjectNomad {
    class ILogger {
    public:
        virtual ~ILogger() {}
        
        #pragma region General Debug Messages

        virtual std::queue<DebugMessage>& getDebugMessages() = 0;
        
        virtual void LogInfoMessage(const std::string& identifier, const std::string& infoMessage) = 0;

        virtual void LogWarnMessage(const std::string& identifier, const std::string& warningMessage) = 0;
        
        virtual void LogErrorMessage(const std::string& identifier, const std::string& errorMessage) = 0;

        virtual void addDebugMessage(DebugMessage debugMessage) = 0;

        virtual void addLogMessage(const std::string& message) = 0;

        virtual void addScreenAndLogMessage(fp displayLength, const std::string& message) = 0;

        virtual void addScreenAndLogMessage(fp displayLength, const std::string& message, LogSeverity logSeverity, OutputColor outputColor) = 0;

        virtual void addShapeMessage(fp displayTime, const FCollider& collider) = 0;

        virtual void addShapeMessage(fp displayTime, const FCollider& collider, OutputColor outputColor) = 0;

        virtual void addBoxMessage(fp displayTime, const FCollider& box) = 0;

        virtual void addBoxMessage(fp displayTime, const FCollider& box, OutputColor outputColor) = 0;

        virtual void addSphereMessage(fp displayTime, const FCollider& sphere) = 0;

        virtual void addSphereMessage(fp displayTime, const FCollider& sphere, OutputColor outputColor) = 0;

        virtual void addCapsuleMessage(fp displayTime, const FCollider& capsule) = 0;

        virtual void addCapsuleMessage(fp displayTime, const FCollider& capsule, OutputColor outputColor) = 0;

#pragma endregion

        #pragma region NetLog Messages

        virtual std::queue<NetLogMessage>& getNetLogMessages() = 0;
        
        virtual void AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color) = 0;

        virtual void AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color, NetLogCategory category) = 0;

        virtual void AddInfoNetLog(const std::string& identifier, const std::string& message) = 0;

        virtual void AddInfoNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) = 0;
        
        virtual void AddWarnNetLog(const std::string& identifier, const std::string& message) = 0;

        virtual void AddWarnNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) = 0;

        virtual void AddErrorNetLog(const std::string& identifier, const std::string& message) = 0;

        virtual void AddErrorNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) = 0;

        #pragma endregion 
    };
}
