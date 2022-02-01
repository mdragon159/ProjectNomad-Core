#pragma once

#include "BaseNetMessage.h"
#include "EOS/EOSWrapperSingleton.h"
#include "EOS/PacketReliability.h"
#include "GameCore/PlayerId.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    enum class NetworkManagerStatus : uint8_t {
        NotInitialized,
        InitializedButNotLoggedIn,
        TryingToLogin,
        LoggedIn,
        ConnectedToPlayer   // Assuming cannot be connected to a session without being logged in
    };

    /// <summary>Interface class to network features</summary>
    class NetworkManagerSingleton {
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
        EOSWrapperSingleton& eosWrapperSingleton = Singleton<EOSWrapperSingleton>::get();

        NetworkManagerStatus managerStatus = NetworkManagerStatus::NotInitialized;

        PlayerId localPlayerId = {};
        PlayerId connectedPlayerId = {};
        
    public:
        #pragma region Lifecycle: Occasionally called methods

        void initialize() {
            if (managerStatus != NetworkManagerStatus::NotInitialized) {
                logger.logWarnMessage(
                    "NetworkManager::initialize",
                    "Called initialize while manager is already initialized"
                );
                return;
            }
            
            if (eosWrapperSingleton.tryInitialize()) {
                managerStatus = NetworkManagerStatus::InitializedButNotLoggedIn;
            }
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
               managerStatus = NetworkManagerStatus::TryingToLogin; 
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
            
            // TODO: Send "TryConnect" message to target player
            // On other side, when message received, send back "AcceptGame" message then set id
            // Finally, on original side, when "AcceptGame" message received then set id
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
                return {};
            }

            return connectedPlayerId;
        }

    private:
        void onMessageReceived() {}

        #pragma region Callbacks from EOSWrapper
        
        void onLoginSuccess(CrossPlatformIdWrapper loggedInCrossPlatformId) {}

        void onConnectionAccepted(CrossPlatformIdWrapper remotePlayerId) {}

        #pragma endregion 
    };
}
