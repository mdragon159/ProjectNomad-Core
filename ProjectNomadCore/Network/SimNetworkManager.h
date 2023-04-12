#pragma once

#include <optional>

#include "INetEventsSubscriber.h"
#include "INetMessageSubscriber.h"
#include "NetMessages.h"
#include "EOS/EOSWrapperSingleton.h"
#include "EOS/PacketReliability.h"
#include "GameCore/PlayerId.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    /**
    * Interface class to network features
    **/
    class SimNetworkManager : public IEOSWrapperManager {
      public:
        ~SimNetworkManager() override  = default;

        bool IsInitialized() const {
            return mIsInitialized;
        }
        bool IsLoggedIn() const {
            return mIsLoggedIn;
        }
        bool IsConnectedToPlayer() const {
            return mIsConnectedToOtherPlayer;
        }
        
        void Initialize() {
            if (IsInitialized()) {
                mLogger.logWarnMessage(
                    "NetworkManager:Initialize",
                    "Called initialize while manager is already initialized"
                );
                return;
            }

            mEosWrapperSingleton.SetWrapperManager(this);
            if (mEosWrapperSingleton.TryInitialize()) {
                mIsInitialized = true;

                if (IsRendererSubscriberSet()) {
                    mRendererSubscriber->get().OnEOSInitialized();
                }
            }
        }

        void OnShutdown() {
            // Nothing to do if was never initialized
            if (!IsInitialized()) {
                return;
            }

            // Clean up state just in case
            CleanupState(false); // Don't actually do a full shutdown as EOS SDK used to not re-init afterwards. See EOSWrapper method comments for more info
        }

        // Expected to be called at least once per frame
        void Tick() {
            // No need to check for initialization or such, just pass the tick down
            mEosWrapperSingleton.Tick();
        }
        

        void RegisterNetMessageSubscriber(INetMessageSubscriber* netMessageSubscriber) {
            mMessageSubscriber = netMessageSubscriber;
        }

        void SetRendererSubscriber(std::reference_wrapper<INetEventsSubscriber> rendererSubscriber) {
            mRendererSubscriber = rendererSubscriber;
        }

        

        void LoginViaAccountPortal() {
            // Sanity checks
            if (CheckAndLogIfNotInitialized("NetworkManagerSingleton::LoginViaAccountPortal")) {
                return;
            }
            if (IsLoggedIn()) {
                mLogger.logWarnMessage(
                    "NetworkManager::LoginViaAccountPortal",
                    "Called while already logged in"
                );
                return;
            }

            mEosWrapperSingleton.BeginLoginAttemptViaAccountPortal();
        }
        
        void LoginViaDevAuth(const std::string& devAuthName, const std::string& ipAndPort) {
            // Sanity checks
            if (CheckAndLogIfNotInitialized("NetworkManagerSingleton::LoginViaDevAuth")) {
                return;
            }
            if (IsLoggedIn()) {
                mLogger.logWarnMessage(
                    "NetworkManager::LoginViaDevAuth",
                    "Called while already logged in"
                );
                return;
            }
            
            if (mEosWrapperSingleton.BeginLoginAttemptViaDevAuthTool(devAuthName, ipAndPort)) {
                // In future, we can set a state to prevent multiple login attempts
                // BUT we'd then need a callback in case login fails
                // So... lazy choice for now is to do nothing here
            }
        }

        void Logout() {
            // Sanity checks
            if (CheckAndLogIfNotInitialized("NetworkManagerSingleton::Logout")) {
                return;
            }
            if (!IsLoggedIn()) {
                mLogger.logWarnMessage(
                    "NetworkManager::Logout",
                    "Called while not logged in"
                );
                return;
            }
            
            mEosWrapperSingleton.BeginLogout();
        }



        bool BeginCreateMainMatchLobby() {
            // Sanity checks
            if (CheckAndLogIfNotInitialized("NetworkManagerSingleton::BeginCreateMainGameLobby")) {
                return false;
            }
            if (!IsLoggedIn()) {
                mLogger.logWarnMessage(
                    "NetworkManager::BeginCreateMainGameLobby",
                    "Called while not yet logged in"
                );
                return false;
            }

            return mEosWrapperSingleton.BeginCreateMainMatchLobby();
        }

        bool BeginLeaveMainMatchLobby() {
            // Sanity checks
            if (CheckAndLogIfNotInitialized("NetworkManagerSingleton::BeginLeaveMainMatchLobby")) {
                return false;
            }
            if (!IsLoggedIn()) {
                mLogger.logWarnMessage(
                    "NetworkManager::BeginLeaveMainMatchLobby",
                    "Called while not yet logged in"
                );
                return false;
            }

            return mEosWrapperSingleton.BeginLeaveMainGameLobby();
        }


        
        void ShowFriendsUI() {
            // Sanity checks
            if (CheckAndLogIfNotInitialized("NetworkManagerSingleton::ShowFriendsUI")) {
                return;
            }
            if (!IsLoggedIn()) {
                mLogger.logWarnMessage(
                    "NetworkManager::ShowFriendsUI",
                    "Called while not logged in"
                );
                return;
            }

            mEosWrapperSingleton.ShowFriendsUI();
        }

        void ConnectToPlayer(const std::string& targetCrossPlatformId) {
            if (!IsLoggedIn()) {
                mLogger.logWarnMessage(
                    "NetworkManager::ConnectToPlayer",
                    "Called while manager is in not in LoggedIn state"
                );
                return;
            }

            InitiateConnectionMessage tryConnectMessage;
            SendMessage(CrossPlatformIdWrapper::fromString(targetCrossPlatformId), tryConnectMessage, PacketReliability::ReliableOrdered);
        }

        void DisconnectFromCurrentSession() {}

        template<typename MessageType>
        void SendMessageToConnectedPlayer(const MessageType& message, PacketReliability packetReliability) {
            static_assert(std::is_base_of_v<BaseNetMessage, MessageType>, "MessageType must inherit from BaseNetMessage");

            if (!IsConnectedToPlayer()) {
                mLogger.logWarnMessage(
                    "NMS::SendMessageToConnectedPlayer",
                    "Called while not connected to a player"
                );
                return;
            }

            SendMessage(mConnectedPlayerId.crossPlatformId, message, packetReliability);
        }

        PlayerId GetLocalPlayerId() {
            return mLocalPlayerId;
        }

        PlayerId GetConnectedPlayerId() {
            if (!IsConnectedToPlayer()) {
                mLogger.logInfoMessage(
                    "NetworkManagerSingleton::GetConnectedPlayerId",
                    "Trying to get connected player id while not connected to anyone"
                );
                return PlayerId();
            }

            return mConnectedPlayerId;
        }



        
        #pragma region EOSWrapper Interface
        void OnLoginSuccess(CrossPlatformIdWrapper loggedInCrossPlatformId) override {
            mLocalPlayerId.crossPlatformId = loggedInCrossPlatformId;
            mIsLoggedIn = true;

            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLoginStatusChanged();
            }
        }
        void OnLogoutSuccess() override {
            ClearLoginState();

            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLoginStatusChanged();
            }
        }
        
        void OnMessageReceived(CrossPlatformIdWrapper peerId, const std::vector<char>& messageData) override {
            if (messageData.empty()) {
                mLogger.addWarnNetLog("NMS::onMessageReceived", "Somehow received empty message...?");
            }
            
            NetMessageType messageType = static_cast<NetMessageType>(messageData[0]);
            switch (messageType) {
                case NetMessageType::TryConnect:
                    SendAcceptConnectionMessage(peerId);
                    RememberAcceptedPlayerConnection(false, peerId);
                    break;

                case NetMessageType::AcceptConnection:
                    RememberAcceptedPlayerConnection(true, peerId);
                    break;

                default:
                    // FUTURE: Verify peer id is the connected player, esp before sending off to other systems
                    if (mMessageSubscriber) {
                        mMessageSubscriber->onMessageReceivedFromConnectedPlayer(messageType, messageData);
                    }
                    else {
                        mLogger.addInfoNetLog(
                            "NMS::onMessageReceived",
                            "No registered message subscriber found, likely a bug"
                        );
                    }
            }
        }

        void OnLobbyCreationResult(bool didSucceed) override {
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLobbyCreationResult(didSucceed);
            }
        }

        void OnLobbyLeftOrDestroyed(bool didSucceed) override {
            if (IsRendererSubscriberSet()) {
                mRendererSubscriber->get().OnLobbyLeftOrDestroyed(didSucceed);
            }
        }
        #pragma endregion 
    
      private:
        bool CheckAndLogIfNotInitialized(const std::string& identifier) const {
            if (IsInitialized()) {
                return false;
            }

            mLogger.logWarnMessage(identifier, "Not initialized");
            return true;
        }
        
        bool IsRendererSubscriberSet() const {
            return mRendererSubscriber.has_value();
        }

        void ClearLoginState() {
            ClearConnectionsIfAny();

            mLocalPlayerId = {};
            mIsLoggedIn = false;
        }
        void ClearConnectionsIfAny() {
            // If don't have any connections then don't have anything to do
            // FUTURE: Expand this as necessary
            if (!IsConnectedToPlayer()) {
                return;
            }

            mIsConnectedToOtherPlayer = false;
            // TODO: Clear actual connection info
        }
        void CleanupState(bool forceShutdown) {
            // Clean up callbacks/subcribers just to be clean
            mMessageSubscriber = nullptr;
            mRendererSubscriber.reset();
            
            // Unlike other typical singletons in project, the EOSWrapper is "managed" by this class
            // Thus pass on the clean up call to the EOSWrapper
            mEosWrapperSingleton.CleanupState(forceShutdown);

            ClearConnectionsIfAny();
            ClearLoginState();
            
            mIsInitialized = false;
        }
        
        template<typename MessageType>
        void SendMessage(CrossPlatformIdWrapper targetId,
                        const MessageType& message,
                        PacketReliability packetReliability) {
            static_assert(std::is_base_of_v<BaseNetMessage, MessageType>, "MessageType must inherit from BaseNetMessage");
            
            mEosWrapperSingleton.SendMessage(
                targetId,
                &message,
                sizeof(message),
                packetReliability
            );
        }

        void RememberAcceptedPlayerConnection(bool didLocalPlayerInitiateConnection, CrossPlatformIdWrapper otherPlayerId) {
            mConnectedPlayerId.crossPlatformId = otherPlayerId;
            
            // Select who's player 1 vs 2 based on who initiated connection
            // This method was chosen as needed *some* consistent way to determine who's which player and this is pretty simple
            if (didLocalPlayerInitiateConnection) {
                mLocalPlayerId.playerSpot = PlayerSpot::Player1;
                mConnectedPlayerId.playerSpot = PlayerSpot::Player2;
            }
            else {
                mLocalPlayerId.playerSpot = PlayerSpot::Player2;
                mConnectedPlayerId.playerSpot = PlayerSpot::Player1;
            }

            mIsConnectedToOtherPlayer = true;
            mLogger.addWarnNetLog(
                "NMS::rememberAcceptedPlayerConnection",
                "Accepted connection!"
            );
        }

        void SendAcceptConnectionMessage(CrossPlatformIdWrapper otherPlayerId) {
            AcceptConnectionMessage acceptConnectionMessage;
            SendMessage(otherPlayerId, acceptConnectionMessage, PacketReliability::ReliableOrdered);
        }

        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();
        EOSWrapperSingleton& mEosWrapperSingleton = Singleton<EOSWrapperSingleton>::get();

        INetMessageSubscriber* mMessageSubscriber = nullptr;
        // Use std::reference_wrapper to be explicit that the subscribers are NOT owned by this class whatsoever.
        // And use std::optional around that for null support (as std::reference_wrapper cannot be null).
        std::optional<std::reference_wrapper<INetEventsSubscriber>> mRendererSubscriber = {};

        bool mIsInitialized = false;
        bool mIsLoggedIn = false;
        bool mIsConnectedToOtherPlayer = false;

        PlayerId mLocalPlayerId = PlayerId(PlayerSpot::Player1);
        PlayerId mConnectedPlayerId = PlayerId(PlayerSpot::INVALID);
    };
}
