#pragma once

#include "Utilities/ILogger.h"

using namespace ProjectNomad;

class TestLogger : public ILogger {
    std::queue<DebugMessage> emptyQueue;
    bool wasMessageLogged = false;

    std::queue<NetLogMessage> emptyNetLogQueue;
    bool wasNetLogMessageLogged = false;

public:
    ~TestLogger() override {}

    bool didLoggingOccur();
    void resetLogging();

#pragma region ILogger Methods
    
    std::queue<DebugMessage>& getDebugMessages() override;
    void LogInfoMessage(const std::string& identifier, const std::string& infoMessage) override;
    void LogWarnMessage(const std::string& identifier, const std::string& warningMessage) override;
    void LogErrorMessage(const std::string& identifier, const std::string& errorMessage) override;
    void addDebugMessage(DebugMessage debugMessage) override;
    void addLogMessage(const std::string& message) override;
    void addScreenAndLogMessage(fp displayLength, const std::string& message) override;
    void addScreenAndLogMessage(fp displayLength,
                                const std::string& message,
                                LogSeverity logSeverity,
                                OutputColor outputColor) override;
    void addShapeMessage(fp displayTime, const Collider& collider) override;
    void addShapeMessage(fp displayTime, const Collider& collider,
        OutputColor outputColor) override;
    void addBoxMessage(fp displayTime, const Collider& box) override;
    void addBoxMessage(fp displayTime, const Collider& box,
        OutputColor outputColor) override;
    void addSphereMessage(fp displayTime, const Collider& sphere) override;
    void addSphereMessage(fp displayTime, const Collider& sphere,
        OutputColor outputColor) override;
    void addCapsuleMessage(fp displayTime, const Collider& capsule) override;
    void addCapsuleMessage(fp displayTime, const Collider& capsule,
        OutputColor outputColor) override;

    std::queue<NetLogMessage>& getNetLogMessages() override;
    void AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color) override;
    void AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color, NetLogCategory category) override;
    void AddInfoNetLog(const std::string& identifier, const std::string& message) override;
    void AddInfoNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) override;
    void AddWarnNetLog(const std::string& identifier, const std::string& message) override;
    void AddWarnNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) override;
    void AddErrorNetLog(const std::string& identifier, const std::string& message) override;
    void AddErrorNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) override;

#pragma endregion 
    
};