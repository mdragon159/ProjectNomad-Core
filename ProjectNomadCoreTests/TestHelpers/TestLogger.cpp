#include "pch.h"
#include "TestLogger.h"

bool TestLogger::didLoggingOccur() {
    return wasMessageLogged;
}

#pragma region ILogger Methods

std::queue<DebugMessage>& TestLogger::getDebugMessages() {
    return emptyQueue;
}

void TestLogger::logInfoMessage(std::string identifier, std::string infoMessage) {
    wasMessageLogged = true;
}

void TestLogger::logWarnMessage(std::string identifier, std::string warningMessage) {
    wasMessageLogged = true;
}

void TestLogger::logErrorMessage(std::string identifier, std::string errorMessage) {
    wasMessageLogged = true;
}

void TestLogger::addDebugMessage(DebugMessage debugMessage) {
    wasMessageLogged = true;
}

void TestLogger::addLogMessage(std::string message) {
    wasMessageLogged = true;
}

void TestLogger::addScreenAndLogMessage(fp displayLength, std::string message) {
    wasMessageLogged = true;
}

void TestLogger::addScreenAndLogMessage(fp displayLength, std::string message, OutputColor outputColor) {
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