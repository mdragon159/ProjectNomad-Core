#pragma once

#include <queue>
#include <source_location>

#include "ILogger.h"
#include "LogHelpers.h"
#include "NetLogMessage.h"
#include "Utilities/DebugMessage.h"
#include "Physics/Model/FCollider.h"

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

        void addShapeMessage(fp displayTime, const FCollider& collider) override {
            if (collider.IsBox()) {
                addBoxMessage(displayTime, collider);
            }
            else if (collider.IsCapsule()) {
                addCapsuleMessage(displayTime, collider);
            }
            else if (collider.IsSphere()) {
                addSphereMessage(displayTime, collider);
            }
        }

        void addShapeMessage(fp displayTime, const FCollider& collider, OutputColor outputColor) override {
            if (collider.IsBox()) {
                addBoxMessage(displayTime, collider, outputColor);
            }
            else if (collider.IsCapsule()) {
                addCapsuleMessage(displayTime, collider, outputColor);
            }
            else if (collider.IsSphere()) {
                addSphereMessage(displayTime, collider, outputColor);
            }
        }

        void addBoxMessage(fp displayTime, const FCollider& box) override {
            if (!box.IsBox()) {
                LogErrorMessage(
                    "SimContext::addBoxMessage",
                    "Provided collider is not a box but of type: " + box.GetTypeAsString()
                );
                return;
            }
            
            debugMessages.push(
                DebugMessage::createBoxMessage(displayTime,
                                        box.GetCenter(),
                                        box.GetBoxHalfSize(),
                                        box.GetRotation()));
        }

        void addBoxMessage(fp displayTime, const FCollider& box, OutputColor outputColor) override {
            if (!box.IsBox()) {
                LogErrorMessage(
                    "SimContext::addBoxMessage",
                    "Provided collider is not a box but of type: " + box.GetTypeAsString()
                );
                return;
            }
            
            debugMessages.push(
                DebugMessage::createBoxMessage(displayTime,
                                        box.GetCenter(),
                                        box.GetBoxHalfSize(),
                                        box.GetRotation(),
                                        outputColor));
        }

        void addSphereMessage(fp displayTime, const FCollider& sphere) override {
            if (!sphere.IsSphere()) {
                LogErrorMessage(
                    "SimContext::addSphereMessage",
                    "Provided collider is not a sphere but of type: " + sphere.GetTypeAsString()
                );
                return;
            }
            
            debugMessages.push(
                DebugMessage::createSphereMessage(displayTime, sphere.GetCenter(), sphere.GetSphereRadius())
            );
        }

        void addSphereMessage(fp displayTime, const FCollider& sphere, OutputColor outputColor) override {
            if (!sphere.IsSphere()) {
                LogErrorMessage(
                    "SimContext::addSphereMessage",
                    "Provided collider is not a sphere but of type: " + sphere.GetTypeAsString()
                );
                return;
            }
            
            debugMessages.push(
                DebugMessage::createSphereMessage(displayTime, sphere.GetCenter(), sphere.GetSphereRadius(), outputColor)
            );
        }

        void addCapsuleMessage(fp displayTime, const FCollider& capsule) override {
            if (!capsule.IsCapsule()) {
                LogErrorMessage(
                    "SimContext::addCapsuleMessage",
                    "Provided collider is not a capsule but of type: " + capsule.GetTypeAsString()
                );
                return;
            }
            
            debugMessages.push(DebugMessage::createCapsuleMessage(
                displayTime, capsule.GetCenter(), capsule.GetCapsuleRadius(), capsule.GetCapsuleHalfHeight(), capsule.GetRotation()
            ));
        }

        void addCapsuleMessage(fp displayTime, const FCollider& capsule, OutputColor outputColor) override {
            if (!capsule.IsCapsule()) {
                LogErrorMessage(
                    "SimContext::addCapsuleMessage",
                    "Provided collider is not a capsule but of type: " + capsule.GetTypeAsString()
                );
                return;
            }
            
            debugMessages.push(DebugMessage::createCapsuleMessage(
                displayTime, capsule.GetCenter(), capsule.GetCapsuleRadius(), capsule.GetCapsuleHalfHeight(),
                capsule.GetRotation(), outputColor
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
