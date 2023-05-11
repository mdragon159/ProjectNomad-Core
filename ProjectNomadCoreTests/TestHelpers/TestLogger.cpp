#include "pchNCT.h"
#include "TestLogger.h"

bool TestLogger::didLoggingOccur() {
    return wasMessageLogged;
}

void TestLogger::resetLogging() {
    // Queues aren't actually used at the moment and thus only need to reset relevant flags
    wasMessageLogged = false;
    wasNetLogMessageLogged = false;
}

#pragma region ILogger Methods

std::queue<DebugMessage>& TestLogger::getDebugMessages() {
    return emptyQueue;
}

void TestLogger::LogInfoMessage(const std::string& identifier, const std::string& infoMessage) {
    wasMessageLogged = true;
}

void TestLogger::LogWarnMessage(const std::string& identifier, const std::string& warningMessage) {
    wasMessageLogged = true;
}

void TestLogger::LogErrorMessage(const std::string& identifier, const std::string& errorMessage) {
    wasMessageLogged = true;
}

void TestLogger::addDebugMessage(DebugMessage debugMessage) {
    wasMessageLogged = true;
}

void TestLogger::addLogMessage(const std::string& message) {
    wasMessageLogged = true;
}

void TestLogger::addScreenAndLogMessage(fp displayLength, const std::string& message) {
    wasMessageLogged = true;
}

void TestLogger::addScreenAndLogMessage(fp displayLength,
                                        const std::string& message,
                                        LogSeverity logSeverity,
                                        OutputColor outputColor) {
    wasMessageLogged = true;
}

void TestLogger::addShapeMessage(fp displayTime, const Collider& collider) {
    wasMessageLogged = true;
}

void TestLogger::addShapeMessage(fp displayTime, const Collider& collider, OutputColor outputColor) {
    wasMessageLogged = true;
}

void TestLogger::addBoxMessage(fp displayTime, const Collider& box) {
    wasMessageLogged = true;
}

void TestLogger::addBoxMessage(fp displayTime, const Collider& box, OutputColor outputColor) {
    wasMessageLogged = true;
}

void TestLogger::addSphereMessage(fp displayTime, const Collider& sphere) {
    wasMessageLogged = true;
}

void TestLogger::addSphereMessage(fp displayTime, const Collider& sphere, OutputColor outputColor) {
    wasMessageLogged = true;
}

void TestLogger::addCapsuleMessage(fp displayTime, const Collider& capsule) {
    wasMessageLogged = true;
}

void TestLogger::addCapsuleMessage(fp displayTime, const Collider& capsule, OutputColor outputColor) {
    wasMessageLogged = true;
}

#pragma endregion

#pragma region NetLog Messages

std::queue<NetLogMessage>& TestLogger::getNetLogMessages() {
    return emptyNetLogQueue;
}

void TestLogger::AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color) {
    wasNetLogMessageLogged = true;
}

void TestLogger::AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color,
    NetLogCategory category) {
    wasNetLogMessageLogged = true;
}

void TestLogger::AddInfoNetLog(const std::string& identifier, const std::string& message) {
    wasNetLogMessageLogged = true;
}

void TestLogger::AddInfoNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) {
    wasNetLogMessageLogged = true;
}

void TestLogger::AddWarnNetLog(const std::string& identifier, const std::string& message) {
    wasNetLogMessageLogged = true;
}

void TestLogger::AddWarnNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) {
    wasNetLogMessageLogged = true;
}

void TestLogger::AddErrorNetLog(const std::string& identifier, const std::string& message) {
    wasNetLogMessageLogged = true;
}

void TestLogger::AddErrorNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) {
    wasNetLogMessageLogged = true;
}

#pragma endregion
