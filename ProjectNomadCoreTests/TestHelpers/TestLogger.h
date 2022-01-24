#pragma once

#include "Utilities/ILogger.h"

using namespace ProjectNomad;

class TestLogger : public ILogger {
    std::queue<DebugMessage> emptyQueue;
    bool wasMessageLogged = false;
    
public:
    ~TestLogger() override {}

    bool didLoggingOccur();

#pragma region ILogger Methods
    
    std::queue<DebugMessage>& getDebugMessages() override;
    void logInfoMessage(std::string identifier, std::string infoMessage) override;
    void logWarnMessage(std::string identifier, std::string warningMessage) override;
    void logErrorMessage(std::string identifier, std::string errorMessage) override;
    void addDebugMessage(DebugMessage debugMessage) override;
    void addLogMessage(std::string message) override;
    void addScreenAndLogMessage(fp displayLength, std::string message) override;
    void addScreenAndLogMessage(fp displayLength, std::string message,
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

#pragma endregion 
    
};