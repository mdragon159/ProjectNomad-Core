#pragma once
#include "ILogger.h"

namespace ProjectNomad {
    class FakeLogger : public ILogger {
        std::queue<DebugMessage> debugMessages;
        std::queue<NetLogMessage> netLogMessages;
    
    public:
        std::queue<DebugMessage>& getDebugMessages() override { return debugMessages; }
        void LogInfoMessage(const std::string& identifier, const std::string& infoMessage) override {}
        void LogWarnMessage(const std::string& identifier, const std::string& warningMessage) override {}
        void LogErrorMessage(const std::string& identifier, const std::string& errorMessage) override {}
        void addDebugMessage(DebugMessage debugMessage) override {}
        void addLogMessage(const std::string& message) override {}
        void addScreenAndLogMessage(fp displayLength, const std::string& message) override {}
        void addScreenAndLogMessage(fp displayLength, const std::string& message, LogSeverity logSeverity, OutputColor outputColor) override {}
        void addShapeMessage(fp displayTime, const FCollider& collider) override {}
        void addShapeMessage(fp displayTime, const FCollider& collider, OutputColor outputColor) override {}
        void addBoxMessage(fp displayTime, const FCollider& box) override {}
        void addBoxMessage(fp displayTime, const FCollider& box, OutputColor outputColor) override {}
        void addSphereMessage(fp displayTime, const FCollider& sphere) override {}
        void addSphereMessage(fp displayTime, const FCollider& sphere, OutputColor outputColor) override {}
        void addCapsuleMessage(fp displayTime, const FCollider& capsule) override {}
        void addCapsuleMessage(fp displayTime, const FCollider& capsule, OutputColor outputColor) override {}
        std::queue<NetLogMessage>& getNetLogMessages() override { return netLogMessages; }
        void AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color) override {}
        void AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color, NetLogCategory category) override {}
        void AddInfoNetLog(const std::string& identifier, const std::string& message) override {}
        void AddInfoNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) override {}
        void AddWarnNetLog(const std::string& identifier, const std::string& message) override {}
        void AddWarnNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) override {}
        void AddErrorNetLog(const std::string& identifier, const std::string& message) override {}
        void AddErrorNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) override {}

    };
}
