#pragma once

#include <queue>
#include <source_location>

#include "ILogger.h"
#include "LogHelpers.h"
#include "NetLogMessage.h"
#include "Utilities/DebugMessage.h"
#include "Physics/Collider.h"

namespace ProjectNomad {
    class LoggerSingleton : public ILogger {
        std::queue<DebugMessage> debugMessages;
        std::queue<NetLogMessage> netLogMessages;
        
      public:
        ~LoggerSingleton() override = default;

        void cleanupState() {
            while(!debugMessages.empty()) {
                debugMessages.pop();
            }
            while(!netLogMessages.empty()) {
                netLogMessages.pop();
            }
        }

        #pragma region General Debug Messages
        
        std::queue<DebugMessage>& getDebugMessages() override {
            return debugMessages;
        }
        
        void LogInfoMessage(const std::string& message, const std::source_location& location = std::source_location::current()) {
            LogInfoMessage(LogHelpers::LocationToString(location), message);
        }
        void LogInfoMessage(const std::string& identifier, const std::string& infoMessage) override {
            std::string message = identifier + ": " + infoMessage;
            addScreenAndLogMessage(fp{0.25f}, message, LogSeverity::Info, OutputColor::White);
        }

        void LogWarnMessage(const std::string& message, const std::source_location& location = std::source_location::current()) {
            LogWarnMessage(LogHelpers::LocationToString(location), message);
        }
        void LogWarnMessage(const std::string& identifier, const std::string& warningMessage) override {
            std::string message = identifier + ": " + warningMessage;
            addScreenAndLogMessage(fp{1.f}, message, LogSeverity::Warn, OutputColor::Orange);
        }

        void LogErrorMessage(const std::string& message, const std::source_location& location = std::source_location::current()) {
            LogErrorMessage(LogHelpers::LocationToString(location), message);
        }
        void LogErrorMessage(const std::string& identifier, const std::string& errorMessage) override {
            std::string message = identifier + ": " + errorMessage;
            addScreenAndLogMessage(fp{5.f}, message, LogSeverity::Error, OutputColor::Red);
        }

        void addDebugMessage(DebugMessage debugMessage) override {
            debugMessages.push(debugMessage);
        }

        void addLogMessage(const std::string& message) override {
            debugMessages.push(DebugMessage::createLogMessage(message));
        }

        void addScreenAndLogMessage(fp displayLength, const std::string& message) override {
            debugMessages.push(
                DebugMessage::createTextMessage(displayLength, OutputLocation::LogAndScreen, message));
        }

        void addScreenAndLogMessage(fp displayLength,
                                    const std::string& message,
                                    LogSeverity logSeverity,
                                    OutputColor outputColor) override {
            debugMessages.push(DebugMessage::createTextMessage(
                displayLength, OutputLocation::LogAndScreen, message, logSeverity, outputColor
            ));
        }

        void addShapeMessage(fp displayTime, const Collider& collider) override {
            if (collider.isBox()) {
                addBoxMessage(displayTime, collider);
            }
            else if (collider.isCapsule()) {
                addCapsuleMessage(displayTime, collider);
            }
            else if (collider.isSphere()) {
                addSphereMessage(displayTime, collider);
            }
        }

        void addShapeMessage(fp displayTime, const Collider& collider, OutputColor outputColor) override {
            if (collider.isBox()) {
                addBoxMessage(displayTime, collider, outputColor);
            }
            else if (collider.isCapsule()) {
                addCapsuleMessage(displayTime, collider, outputColor);
            }
            else if (collider.isSphere()) {
                addSphereMessage(displayTime, collider, outputColor);
            }
        }

        void addBoxMessage(fp displayTime, const Collider& box) override {
            if (!box.isBox()) {
                LogErrorMessage(
                    "SimContext::addBoxMessage",
                    "Provided collider is not a box but of type: " + box.getTypeAsString()
                );
                return;
            }
            
            debugMessages.push(
                DebugMessage::createBoxMessage(displayTime,
                                        box.getCenter(),
                                        box.getBoxHalfSize(),
                                        box.getRotation()));
        }

        void addBoxMessage(fp displayTime, const Collider& box, OutputColor outputColor) override {
            if (!box.isBox()) {
                LogErrorMessage(
                    "SimContext::addBoxMessage",
                    "Provided collider is not a box but of type: " + box.getTypeAsString()
                );
                return;
            }
            
            debugMessages.push(
                DebugMessage::createBoxMessage(displayTime,
                                        box.getCenter(),
                                        box.getBoxHalfSize(),
                                        box.getRotation(),
                                        outputColor));
        }

        void addSphereMessage(fp displayTime, const Collider& sphere) override {
            if (!sphere.isSphere()) {
                LogErrorMessage(
                    "SimContext::addSphereMessage",
                    "Provided collider is not a sphere but of type: " + sphere.getTypeAsString()
                );
                return;
            }
            
            debugMessages.push(
                DebugMessage::createSphereMessage(displayTime, sphere.getCenter(), sphere.getSphereRadius())
            );
        }

        void addSphereMessage(fp displayTime, const Collider& sphere, OutputColor outputColor) override {
            if (!sphere.isSphere()) {
                LogErrorMessage(
                    "SimContext::addSphereMessage",
                    "Provided collider is not a sphere but of type: " + sphere.getTypeAsString()
                );
                return;
            }
            
            debugMessages.push(
                DebugMessage::createSphereMessage(displayTime, sphere.getCenter(), sphere.getSphereRadius(), outputColor)
            );
        }

        void addCapsuleMessage(fp displayTime, const Collider& capsule) override {
            if (!capsule.isCapsule()) {
                LogErrorMessage(
                    "SimContext::addCapsuleMessage",
                    "Provided collider is not a capsule but of type: " + capsule.getTypeAsString()
                );
                return;
            }
            
            debugMessages.push(DebugMessage::createCapsuleMessage(
                displayTime, capsule.getCenter(), capsule.getCapsuleRadius(), capsule.getCapsuleHalfHeight(), capsule.getRotation()
            ));
        }

        void addCapsuleMessage(fp displayTime, const Collider& capsule, OutputColor outputColor) override {
            if (!capsule.isCapsule()) {
                LogErrorMessage(
                    "SimContext::addCapsuleMessage",
                    "Provided collider is not a capsule but of type: " + capsule.getTypeAsString()
                );
                return;
            }
            
            debugMessages.push(DebugMessage::createCapsuleMessage(
                displayTime, capsule.getCenter(), capsule.getCapsuleRadius(), capsule.getCapsuleHalfHeight(),
                capsule.getRotation(), outputColor
            ));
        }

        #pragma endregion

        #pragma region NetLog Messages

        std::queue<NetLogMessage>& getNetLogMessages() override {
            return netLogMessages;
        }
        
        void AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color) override {
            AddNetLogMessage(message, logSeverity, color, NetLogCategory::SimLayer); // Assume SimLayer by default
        }
        void AddNetLogMessage(const std::string& message, LogSeverity logSeverity, OutputColor color, NetLogCategory category) override {
            netLogMessages.push({message, logSeverity, color, category});
        }

        void AddInfoNetLog(const std::string& message, const std::source_location& location = std::source_location::current()) {
            AddInfoNetLog(LogHelpers::LocationToString(location), message);
        }
        void AddInfoNetLog(const std::string& identifier, const std::string& message) override {
            const std::string& formattedMsg = identifier + ": " + message;
            AddNetLogMessage(formattedMsg, LogSeverity::Info, OutputColor::White);
        }
        void AddInfoNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) override {
            const std::string& formattedMsg = identifier + ": " + message;
            AddNetLogMessage(formattedMsg, LogSeverity::Info, OutputColor::White, category);
        }

        void AddWarnNetLog(const std::string& message, const std::source_location& location = std::source_location::current()) {
            AddWarnNetLog(LogHelpers::LocationToString(location), message);
        }
        void AddWarnNetLog(const std::string& identifier, const std::string& message) override {
            const std::string& formattedMsg = identifier + ": " + message;
            AddNetLogMessage(formattedMsg, LogSeverity::Warn, OutputColor::Orange);
        }
        void AddWarnNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) override {
            const std::string& formattedMsg = identifier + ": " + message;
            AddNetLogMessage(formattedMsg, LogSeverity::Warn, OutputColor::Orange, category);
        }

        void AddErrorNetLog(const std::string& message, const std::source_location& location = std::source_location::current()) {
            AddErrorNetLog(LogHelpers::LocationToString(location), message);
        }
        void AddErrorNetLog(const std::string& identifier, const std::string& message) override {
            const std::string& formattedMsg = identifier + ": " + message;
            AddNetLogMessage(formattedMsg, LogSeverity::Error, OutputColor::Red);
        }
        void AddErrorNetLog(const std::string& identifier, const std::string& message, NetLogCategory category) override {
            const std::string& formattedMsg = identifier + ": " + message;
            AddNetLogMessage(formattedMsg, LogSeverity::Error, OutputColor::Red, category);
        }
        
        #pragma endregion
    };
}
