#pragma once

#include "INetMessageSubscriber.h"
#include "NetMessages.h"
#include "EOS/EOSWrapperSingleton.h"
#include "EOS/PacketReliability.h"
#include "GameCore/PlayerId.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    enum class NetworkManagerStatus : uint8_t {
        NotInitialized,
        InitializedButNotLoggedIn,
        LoggedIn,
        ConnectedToPlayer   // Assuming cannot be connected to a session without being logged in
    };

    /// <summary>Interface class to network features</summary>
    class NetworkManagerSingleton : public IEOSWrapperManager {
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
        EOSWrapperSingleton& eosWrapperSingleton = Singleton<EOSWrapperSingleton>::get();

        INetMessageSubscriber* messageSubscriber = nullptr;

        NetworkManagerStatus managerStatus = NetworkManagerStatus::NotInitialized;

        PlayerId localPlayerId = PlayerId(PlayerSpot::Player1);
        PlayerId connectedPlayerId = PlayerId(PlayerSpot::INVALID);
        
    public:
        ~NetworkManagerSingleton() override {}
        
        #pragma region Lifecycle: Occasionally called methods

        void initialize() {
            if (managerStatus != NetworkManagerStatus::NotInitialized) {
                logger.logWarnMessage(
                    "NetworkManager::initialize",
                    "Called initialize while manager is already initialized"
                );
                return;
            }

            eosWrapperSingleton.setWrapperManager(this);
            if (eosWrapperSingleton.tryInitialize()) {
                managerStatus = NetworkManagerStatus::InitializedButNotLoggedIn;
            }
        }

        void registerNetMessageSubscriber(INetMessageSubscriber* netMessageSubscriber) {
            messageSubscriber = netMessageSubscriber;
        }

        void loginViaDevAuth(const std::string& devAuthName) {
            if (managerStatus != NetworkManagerStatus::InitializedButNotLoggedIn) {
                logger.logWarnMessage(
                    "NetworkManager::loginViaDevAuth",
                    "Called while manager is in not in InitializedButNotLoggedIn state"
                );
                return;
            }
            
            if (eosWrapperSingleton.tryBeginLoginAttempt(devAuthName)) {
                // In future, we can set a state to prevent multiple login attempts
                // BUT we'd then need a callback in case login fails
                // So... lazy choice for now is to do nothing here
            }
        }

        void connectToPlayer(const std::string& targetCrossPlatformId) {
            if (managerStatus != NetworkManagerStatus::LoggedIn) {
                logger.logWarnMessage(
                    "NetworkManager::connectToPlayer",
                    "Called while manager is in not in LoggedIn state"
                );
                return;
            }

            InitiateConnectionMessage tryConnectMessage;
            sendMessage(CrossPlatformIdWrapper::fromString(targetCrossPlatformId), tryConnectMessage, PacketReliability::ReliableOrdered);
        }

        void disconnectFromCurrentSession() {}
        
        void logout() {
            if (managerStatus != NetworkManagerStatus::LoggedIn) {
                logger.logWarnMessage(
                    "NetworkManager::loginViaDevAuth",
                    "Called while manager is in not in LoggedIn state"
                );
                return;
            }
            
            eosWrapperSingleton.logout();
        }

        void cleanupState(bool forceShutdown) {
            messageSubscriber = nullptr;
            
            // Unlike other typical singletons in project, the EOSWrapper is "managed" by this class
            // Thus pass on the clean up call to the EOSWrapper
            eosWrapperSingleton.cleanupState(forceShutdown);
        }
        
        #pragma endregion

        // Expected to be called at least once per frame
        void update() {
            eosWrapperSingleton.update();
        }

        template<typename MessageType>
        void sendMessageToConnectedPlayer(const MessageType& message, PacketReliability packetReliability) {
            static_assert(std::is_base_of_v<BaseNetMessage, MessageType>, "MessageType must inherit from BaseNetMessage");

            if (managerStatus != NetworkManagerStatus::ConnectedToPlayer) {
                logger.logWarnMessage(
                    "NMS::sendMessageToConnectedPlayer",
                    "Called while not connected to a player"
                );
                return;
            }

            sendMessage(connectedPlayerId.crossPlatformId, message, packetReliability);
        }

        PlayerId getLocalPlayerId() {
            return localPlayerId;
        }

        PlayerId getConnectedPlayerId() {
            if (managerStatus != NetworkManagerStatus::ConnectedToPlayer) {
                logger.logInfoMessage(
                    "NetworkManagerSingleton::getConnectedPlayerId",
                    "Trying to get connected player id while not connected to anyone"
                );
                return PlayerId();
            }

            return connectedPlayerId;
        }

        bool isConnectedToPlayer() {
            return managerStatus == NetworkManagerStatus::ConnectedToPlayer;
        }

        #pragma region EOSWrapper Interface

        void onLoginSuccess(CrossPlatformIdWrapper loggedInCrossPlatformId) override {
            managerStatus = NetworkManagerStatus::LoggedIn;
            localPlayerId.crossPlatformId = loggedInCrossPlatformId;
        }
        
        void onMessageReceived(CrossPlatformIdWrapper peerId, const std::vector<char>& messageData) override {
            if (messageData.empty()) {
                logger.addWarnNetLog("NMS::onMessageReceived", "Somehow received empty message...?");
            }
            
            NetMessageType messageType = static_cast<NetMessageType>(messageData[0]);
            switch (messageType) {
                case NetMessageType::TryConnect:
                    sendAcceptConnectionMessage(peerId);
                    rememberAcceptedPlayerConnection(false, peerId);
                    break;

                case NetMessageType::AcceptConnection:
                    rememberAcceptedPlayerConnection(true, peerId);
                    break;

                default:
                    // FUTURE: Verify peer id is the connected player, esp before sending off to other systems
                    if (messageSubscriber) {
                        messageSubscriber->onMessageReceivedFromConnectedPlayer(messageType, messageData);
                    }
                    else {
                        logger.addInfoNetLog(
                            "NMS::onMessageReceived",
                            "No registered message subscriber found, likely a bug"
                        );
                    }
            }
            
            /*// For now, simply output message
            std::string messageAsString(messageData.begin(), messageData.end());
            logger.addInfoNetLog("NMS::onMessageReceived", "Received message: " + messageAsString);

            // Also for now output the peer id
            std::string peerIdAsString;
            if (!peerId.tryToString(peerIdAsString)) {
                logger.addErrorNetLog("NMS::onMessageReceived", "Failed to convert id to string");
                return;
            }
            logger.addInfoNetLog("NMS::onMessageReceived", "Peer id is: " + peerIdAsString);*/
        }

        #pragma endregion 
    
    private:
        template<typename MessageType>
        void sendMessage(CrossPlatformIdWrapper targetId,
                        const MessageType& message,
                        PacketReliability packetReliability) {
            static_assert(std::is_base_of_v<BaseNetMessage, MessageType>, "MessageType must inherit from BaseNetMessage");
            
            eosWrapperSingleton.sendMessage(
                targetId,
                &message,
                sizeof(message),
                packetReliability
            );
        }

        void rememberAcceptedPlayerConnection(bool didLocalPlayerInitiateConnection, CrossPlatformIdWrapper otherPlayerId) {
            connectedPlayerId.crossPlatformId = otherPlayerId;
            
            // Select who's player 1 vs 2 based on who initiated connection
            // This method was chosen as needed *some* consistent way to determine who's which player and this is pretty simple
            if (didLocalPlayerInitiateConnection) {
                localPlayerId.playerSpot = PlayerSpot::Player1;
                connectedPlayerId.playerSpot = PlayerSpot::Player2;
            }
            else {
                localPlayerId.playerSpot = PlayerSpot::Player2;
                connectedPlayerId.playerSpot = PlayerSpot::Player1;
            }

            managerStatus = NetworkManagerStatus::ConnectedToPlayer;
            logger.addWarnNetLog(
                "NMS::rememberAcceptedPlayerConnection",
                "Accepted connection!"
            );
        }

        void sendAcceptConnectionMessage(CrossPlatformIdWrapper otherPlayerId) {
            AcceptConnectionMessage acceptConnectionMessage;
            sendMessage(otherPlayerId, acceptConnectionMessage, PacketReliability::ReliableOrdered);
        }
    };
}
