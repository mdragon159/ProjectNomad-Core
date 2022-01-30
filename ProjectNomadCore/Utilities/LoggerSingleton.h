#pragma once
#include <queue>

#include "ILogger.h"
#include "NetLogMessage.h"
#include "Utilities/DebugMessage.h"
#include "Physics/Collider.h"

namespace ProjectNomad {
    class LoggerSingleton : public ILogger {
        std::queue<DebugMessage> debugMessages;
        std::queue<NetLogMessage> netLogMessages;
        
    public:
        ~LoggerSingleton() override {}

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
        
        void logInfoMessage(std::string identifier, std::string infoMessage) override {
            std::string message = identifier + ": " + infoMessage;
            addScreenAndLogMessage(fp{0.25f}, message, OutputColor::White);
        }

        void logWarnMessage(std::string identifier, std::string warningMessage) override {
            std::string message = identifier + ": " + warningMessage;
            addScreenAndLogMessage(fp{1.f}, message, OutputColor::Orange);
        }
        
        void logErrorMessage(std::string identifier, std::string errorMessage) override {
            std::string message = identifier + ": " + errorMessage;
            addScreenAndLogMessage(fp{5.f}, message, OutputColor::Red);
        }

        void addDebugMessage(DebugMessage debugMessage) override {
            debugMessages.push(debugMessage);
        }

        void addLogMessage(std::string message) override {
            debugMessages.push(DebugMessage::createLogMessage(message));
        }

        void addScreenAndLogMessage(fp displayLength, std::string message) override {
            debugMessages.push(
                DebugMessage::createTextMessage(displayLength, OutputLocation::LogAndScreen, message));
        }

        void addScreenAndLogMessage(fp displayLength, std::string message, OutputColor outputColor) override {
            debugMessages.push(
                DebugMessage::createTextMessage(displayLength, OutputLocation::LogAndScreen, message, outputColor));
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
                logErrorMessage(
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
                logErrorMessage(
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
                logErrorMessage(
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
                logErrorMessage(
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
                logErrorMessage(
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
                logErrorMessage(
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
        
        void addNetLogMessage(std::string message) override {
            netLogMessages.push(NetLogMessage(message));
            addLogMessage(message);
        }

        void addNetLogMessage(std::string message, NetLogCategory category) override {
            netLogMessages.push(NetLogMessage(message, category));
            addLogMessage(message);
        }

        void addNetLogMessage(std::string message, OutputColor color) override {
            netLogMessages.push(NetLogMessage(message, color));
            addLogMessage(message);
        }

        void addNetLogMessage(std::string message, OutputColor color, NetLogCategory category) override {
            netLogMessages.push(NetLogMessage(message, color, category));
            addLogMessage(message);
        }

        void addInfoNetLog(std::string identifier, std::string message) override {
            std::string formattedMsg = identifier + ": " + message;
            addNetLogMessage(formattedMsg, OutputColor::White);
        }

        void addInfoNetLog(std::string identifier, std::string message, NetLogCategory category) override {
            std::string formattedMsg = identifier + ": " + message;
            addNetLogMessage(formattedMsg, OutputColor::White, category);
        }
        
        void addWarnNetLog(std::string identifier, std::string message) override {
            std::string formattedMsg = identifier + ": " + message;
            addNetLogMessage(formattedMsg, OutputColor::Orange);
        }

        void addWarnNetLog(std::string identifier, std::string message, NetLogCategory category) override {
            std::string formattedMsg = identifier + ": " + message;
            addNetLogMessage(formattedMsg, OutputColor::Orange, category);
        }

        void addErrorNetLog(std::string identifier, std::string message) override {
            std::string formattedMsg = identifier + ": " + message;
            addNetLogMessage(formattedMsg, OutputColor::Red);
        }

        void addErrorNetLog(std::string identifier, std::string message, NetLogCategory category) override {
            std::string formattedMsg = identifier + ": " + message;
            addNetLogMessage(formattedMsg, OutputColor::Red, category);
        }
        
        #pragma endregion 
    };
}
