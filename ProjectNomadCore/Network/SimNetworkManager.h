#pragma once

#include "INetEventsSubscriber.h"
#include "EOS/EOSWrapperSingleton.h"
#include "EOS/Model/PacketReliability.h"
#include "GameCore/PlayerId.h"
#include "Model/NetPlayersInfoManager.h"
#include "Model/NetSubscribersManager.h"
#include "P2PMessages/NetMessagesPlayerSpot.h"
#include "P2PMessages/NetMessagesSimple.h"
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
            return mPlayersInfoManager.GetPlayersInfo().isLoggedIn;
        }
        bool IsInLobby() const {
            return GetPlayersInfo().netLobbyInfo.isInLobby;
        }
        const NetAllPlayersInfo& GetPlayersInfo() const {
            return mPlayersInfoManager.GetPlayersInfo();
        }
        
        void Initialize() {
            if (IsInitialized()) {
                mLogger.AddWarnNetLog("Called initialize while manager is already initialized");
                return;
            }

            mEosWrapperSingleton.SetWrapperManager(this);
            if (mEosWrapperSingleton.TryInitialize()) {
                mIsInitialized = true;

                mNetSubscribersManager.OnEOSInitialized();
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
        // Expected to be called once per frame
        void Tick() {
            // No need to check for initialization or such, just pass the tick down
            mEosWrapperSingleton.Tick();
        }
        
        void SetRendererSubscriber(std::reference_wrapper<INetEventsSubscriber> subscriber) {
            mNetSubscribersManager.SetRendererSubscriber(subscriber);
        }
        void SetSimGameSubscriber(std::reference_wrapper<INetEventsSubscriber> subscriber) {
            mNetSubscribersManager.SetSimGameSubscriber(subscriber);
        }
        
        void LoginViaAccountPortal() {
            // Sanity checks
            if (CheckAndLogIfNotInitialized("NetworkManagerSingleton::LoginViaAccountPortal")) {
                return;
            }
            if (IsLoggedIn()) {
                mLogger.AddWarnNetLog("Called while already logged in");
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
                mLogger.AddWarnNetLog("Called while already logged in");
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
                mLogger.AddWarnNetLog("Called while already logged in");
                return;
            }
            
            mEosWrapperSingleton.BeginLogout();
        }

        /**
        * Send a P2P message to all players in the current match lobby
        * @tparam MessageType - Message type to send. Must extend BaseNetMessage
        * @param message - Message to send
        * @param packetReliability - How should message be sent. This will dictate whether in UDP-style, TCP-style, etc
        * @returns true if message successfully queued for sending to all players 
        **/
        template<typename MessageType>
        bool SendP2PMessageToAllPlayersInMatchLobby(const MessageType& message, PacketReliability packetReliability) {
            static_assert(std::is_base_of_v<BaseNetMessage, MessageType>, "MessageType must inherit from BaseNetMessage");

            // General sanity checks
            if (CheckAndLogIfNotInitialized("SNM::SendP2PMessageToAllPlayersInMatchLobby")) {
                return false;
            }
            if (!IsLoggedIn()) {
                mLogger.AddWarnNetLog("SNM::SendP2PMessageToAllPlayersInMatchLobby", "Called while not yet logged in");
                return false;
            }
            // Lobby sanity checks
            const NetAllPlayersInfo& allPlayersInfo = GetPlayersInfo(); // For readability
            if (!allPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Called while not actually in a lobby");
                return false;
            }
            if (allPlayersInfo.netLobbyInfo.lobbyMemberIds.size() <= 1) {
                mLogger.AddWarnNetLog("Called while only player in lobby");
                return false;
            }

            // Send the message to each player one by one
            bool didAnySendsFail = false;
            for (const CrossPlatformIdWrapper& memberId : allPlayersInfo.netLobbyInfo.lobbyMemberIds) {
                // No need to even attempt sending message to self
                if (memberId == allPlayersInfo.localPlayerId) {
                    continue;
                }

                // Actually send the message
                bool didSendSucceed = SendP2PMessage(memberId, message, packetReliability);
                if (!didSendSucceed) {
                    mLogger.AddWarnNetLog(
                        "SNM::SendP2PMessageToAllPlayersInMatchLobby",
                        "Failed to send message to player with id: " + memberId.ToStringForLogging()
                    );
                    didAnySendsFail = true;
                }
            }

            return !didAnySendsFail;
        }
        /**
        * Send a P2P message to match lobby's host
        * @tparam MessageType - Message type to send. Must extend BaseNetMessage
        * @param message - Message to send
        * @param packetReliability - How should message be sent. This will dictate whether in UDP-style, TCP-style, etc
        * @returns true if message successfully queued for sending to host player
        **/
        template<typename MessageType>
        bool SendP2PMessageToMatchLobbyHost(const MessageType& message, PacketReliability packetReliability) {
            // General sanity checks
            if (CheckAndLogIfNotInitialized("SNM::SendP2PMessageToMatchLobbyHost")) {
                return false;
            }
            if (!IsLoggedIn()) {
                mLogger.AddWarnNetLog("Called while not yet logged in");
                return false;
            }
            // Lobby sanity checks
            const NetAllPlayersInfo& allPlayersInfo = GetPlayersInfo(); // For readability
            if (!allPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Called while not actually in a lobby");
                return false;
            }
            if (allPlayersInfo.netLobbyInfo.isLocalPlayerLobbyOwner) {
                mLogger.AddWarnNetLog( "Called while local player is actually host");
                return false;
            }

            // Actually send the message
            return SendP2PMessage(allPlayersInfo.netLobbyInfo.lobbyOwner, message, packetReliability);
        }

        bool BeginCreateMainMatchLobby() {
            // Sanity checks
            if (CheckAndLogIfNotInitialized("NetworkManagerSingleton::BeginCreateMainGameLobby")) {
                return false;
            }
            if (!IsLoggedIn()) {
                mLogger.AddWarnNetLog("Called while not yet logged in");
                return false;
            }
            if (IsInLobby()) {
                mLogger.AddWarnNetLog("Called while already in a lobby");
                return false;
            }

            return mEosWrapperSingleton.BeginCreateMainMatchLobby();
        }
        bool BeginLeaveMainMatchLobby() {
            // Sanity checks
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Not initialized");
                return false;
            }
            if (!IsLoggedIn()) {
                mLogger.AddWarnNetLog("Called while not yet logged in");
                return false;
            }
            if (!IsInLobby()) {
                mLogger.AddWarnNetLog("Called while not actually in a lobby");
                return false;
            }

            return mEosWrapperSingleton.BeginLeaveMainGameLobby();
        }
        
        void LockLobby() {
            // Sanity checks
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Not initialized");
                return;
            }
            if (!IsLoggedIn()) {
                mLogger.AddWarnNetLog("Called while not yet logged in");
                return;
            }
            if (!IsInLobby()) {
                mLogger.AddWarnNetLog("Called while not actually in a lobby");
                return;
            }

            mPlayersInfoManager.LockLobby(); // See comments of this method for why this is important to do
        }
        void UnlockLobby() {
            // Sanity checks
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Not initialized");
                return;
            }
            if (!IsLoggedIn()) {
                mLogger.AddWarnNetLog("Called while not yet logged in");
                return;
            }
            if (!IsInLobby()) {
                mLogger.AddWarnNetLog("Called while not actually in a lobby");
                return;
            }

            mPlayersInfoManager.UnlockLobby();
        }
        
        void ShowFriendsUI() {
            // Sanity checks
            if (CheckAndLogIfNotInitialized("NetworkManagerSingleton::ShowFriendsUI")) {
                return;
            }
            if (!IsLoggedIn()) {
                mLogger.AddWarnNetLog("Called while not logged in");
                return;
            }

            mEosWrapperSingleton.ShowFriendsUI();
        }

        // TODO: Clean up or rework outdated "connection" tracking and logic
        bool IsConnectedToPlayer() const { // TODO: Delete this and clean up the one caller
            return mIsConnectedToOtherPlayer;
        }
        void ConnectToPlayer(const std::string& targetCrossPlatformId) {
            if (!IsLoggedIn()) {
                mLogger.AddWarnNetLog("Called while manager is in not in LoggedIn state");
                return;
            }

            InitiateConnectionMessage tryConnectMessage;
            SendP2PMessage(CrossPlatformIdWrapper::FromString(targetCrossPlatformId), tryConnectMessage, PacketReliability::ReliableOrdered);
        }
        template<typename MessageType>
        void SendMessageToConnectedPlayer(const MessageType& message, PacketReliability packetReliability) {
            static_assert(std::is_base_of_v<BaseNetMessage, MessageType>, "MessageType must inherit from BaseNetMessage");

            if (!IsConnectedToPlayer()) {
                mLogger.AddWarnNetLog("Called while not connected to a player");
                return;
            }

            SendP2PMessage(mConnectedPlayerId.crossPlatformId, message, packetReliability);
        }
        
        #pragma region EOSWrapper Interface
        void OnLoginSuccess(const CrossPlatformIdWrapper& loggedInCrossPlatformId) override {
            mPlayersInfoManager.OnLoggedIn(loggedInCrossPlatformId);

            mNetSubscribersManager.OnLoginStatusChanged();
        }
        void OnLogoutSuccess() override {
            mPlayersInfoManager.OnLoggedOut();

            mNetSubscribersManager.OnLoginStatusChanged();
        }
        
        void OnMessageReceived(const CrossPlatformIdWrapper& senderId, const std::vector<char>& messageData) override {
            if (messageData.empty()) {
                mLogger.AddWarnNetLog("Somehow received empty message...?");
                return;
            }
            
            // First byte should always be the message identifier byte
            if (!IsValidMessageType(messageData[0])) {
                mLogger.AddWarnNetLog(
                    "Invalid first identifying byte: " + std::to_string(messageData[0]) // to_string unnecessary atm with char type but I likey consistency
                );
                return;
            }
            NetMessageType messageType = static_cast<NetMessageType>(messageData[0]);

            // Handle depending on message type
            //      NOTE: All downstream message casts should validate message size before proceeding
            //      TODO: Would be verrry nice if we could somehow always validate here for downstream
            switch (messageType) {
                // TODO: Clean up or rework old message types!
                case NetMessageType::TryConnect:
                    SendAcceptConnectionMessage(senderId);
                    RememberAcceptedPlayerConnection(false, senderId);
                    break;
                case NetMessageType::AcceptConnection:
                    RememberAcceptedPlayerConnection(true, senderId);
                    break;

                case NetMessageType::PlayerSpotMapping: {
                    if (messageData.size() != sizeof(PlayerSpotMappingMessage)) { // Could be wrong size from bad actors
                        mLogger.AddWarnNetLog(
                            "PlayerSpotMapping: Received invalid message size! Given size: " +
                            std::to_string(messageData.size()) + ", expected size: " + std::to_string(sizeof(PlayerSpotMappingMessage))
                        );
                        return;
                    }
                
                    const auto* actualMessage = reinterpret_cast<const PlayerSpotMappingMessage*>(messageData.data());
                    ProcessReceivedPlayerSpotMapping(senderId, *actualMessage);
                    break;
                }

                default:
                    // Propagate message to subscribers to handle as they see fit
                    mNetSubscribersManager.HandleReceivedP2PMessage(senderId, messageType, messageData);
            }
        }

        void OnAllPlayerInfoQueriesCompleted(const EOSPlayersInfoTracking& playersInfoTracking) override {
            // Update any necessary tracked info before downstream consuming code needs it
            mPlayersInfoManager.UpdateLobbyMembersGeneralInfo(playersInfoTracking);

            mNetSubscribersManager.OnAllPlayerInfoQueriesCompleted();
        }
        
        void OnLobbyCreationResult(bool didSucceed,
                                   const EOSLobbyProperties& lobbyProperties,
                                   const EOSPlayersInfoTracking& playersInfoTracking) override {
            // Update relevant info so subscribers can easily retrieve the info
            mPlayersInfoManager.OnLobbyJoinOrCreationResult(didSucceed, lobbyProperties, playersInfoTracking);

            // Notify subscribers of event
            mNetSubscribersManager.OnLobbyCreationResult(didSucceed);

            SendPlayerSpotMappingIfAppropriate(didSucceed);
        }
        void OnLobbyJoinResult(bool didSucceed,
                               const EOSLobbyProperties& lobbyProperties,
                               const EOSPlayersInfoTracking& playersInfoTracking) override {
            // Update relevant info so subscribers can easily retrieve the info
            mPlayersInfoManager.OnLobbyJoinOrCreationResult(didSucceed, lobbyProperties, playersInfoTracking);

            // Notify subscribers of event
            mNetSubscribersManager.OnLobbyJoinResult(didSucceed);

            SendPlayerSpotMappingIfAppropriate(didSucceed);
        }
        void OnLobbyLeftOrDestroyed(bool didSucceed) override {
            mPlayersInfoManager.OnLobbyLeftOrDestroyed(didSucceed);

            mNetSubscribersManager.OnLobbyLeftOrDestroyed(didSucceed);
        }
        void OnLobbyUpdated(bool didSucceed,
                            const EOSLobbyProperties& lobbyProperties,
                            const EOSPlayersInfoTracking& playersInfoTracking) override {
            mPlayersInfoManager.OnLobbyUpdated(didSucceed, lobbyProperties, playersInfoTracking);

            mNetSubscribersManager.OnLobbyUpdated();

            SendPlayerSpotMappingIfAppropriate(didSucceed);
        }

        void OnLobbyJoinOrCreateBegin() override {
            // Simply forward the event to the "frontend"
            mNetSubscribersManager.OnLobbyJoinOrCreateBegin();
        }
        void OnLobbyLeaveBegin() override {
            // Simply forward the event to the "frontend"
            mNetSubscribersManager.OnLobbyLeaveBegin();
        }
        #pragma endregion 
    
      private:
        bool CheckAndLogIfNotInitialized(const std::string& identifier) const {
            if (IsInitialized()) {
                return false;
            }

            mLogger.AddWarnNetLog(identifier, "Not initialized");
            return true;
        }
        
        void CleanupState(bool forceShutdown) {
            // Clean up callbacks/subcribers just to be clean and safe
            mNetSubscribersManager = {};

            // FUTURE: Clear up any actual data storage we have (like logged in status). However...
            //      This only matters if we support re-initialization which isn't a goal atm.
            //      Thus, we'll only worry about more "proper" clean up only if we need to support re-init case.
            
            // Unlike other typical singletons in project, the EOSWrapper is "managed" by this class
            // Thus pass on the clean up call to the EOSWrapper
            mEosWrapperSingleton.CleanupState(forceShutdown);
            
            mIsInitialized = false;
        }
        
        template<typename MessageType>
        bool SendP2PMessage(const CrossPlatformIdWrapper& targetId,
                            const MessageType& message,
                            PacketReliability packetReliability) {
            static_assert(std::is_base_of_v<BaseNetMessage, MessageType>, "MessageType must inherit from BaseNetMessage");
            
            return mEosWrapperSingleton.SendP2PMessage(
                targetId,
                &message,
                sizeof(message),
                packetReliability
            );
        }
        static bool IsValidMessageType(uint8_t input) {
            static constexpr int finalEnumVal = static_cast<size_t>(NetMessageType::ENUM_COUNT) - 1;
            return input <= finalEnumVal;
        }

        // TODO: Clean up or rework all this old p2p "connection" tracking + logic
        void ClearConnectionsIfAny() {
            // If don't have any connections then don't have anything to do
            // FUTURE: Expand this as necessary
            if (!IsConnectedToPlayer()) {
                return;
            }

            mIsConnectedToOtherPlayer = false;
            // TODO: Clear actual connection info
        }
        void RememberAcceptedPlayerConnection(bool didLocalPlayerInitiateConnection, CrossPlatformIdWrapper otherPlayerId) {
            mConnectedPlayerId.crossPlatformId = otherPlayerId;

            // TODO: Redo all this connection-related code
            
            /*// Select who's player 1 vs 2 based on who initiated connection
            // This method was chosen as needed *some* consistent way to determine who's which player and this is pretty simple
            if (didLocalPlayerInitiateConnection) {
                mLocalPlayerId.playerSpot = PlayerSpot::Player1;
                mConnectedPlayerId.playerSpot = PlayerSpot::Player2;
            }
            else {
                mLocalPlayerId.playerSpot = PlayerSpot::Player2;
                mConnectedPlayerId.playerSpot = PlayerSpot::Player1;
            }

            mIsConnectedToOtherPlayer = true;*/
            mLogger.AddWarnNetLog("Accepted connection!");
        }
        void SendAcceptConnectionMessage(CrossPlatformIdWrapper otherPlayerId) {
            AcceptConnectionMessage acceptConnectionMessage;
            SendP2PMessage(otherPlayerId, acceptConnectionMessage, PacketReliability::ReliableOrdered);
        }

        void SendPlayerSpotMappingIfAppropriate(bool didSucceed) {
            // Goal: Host should always send player spot mapping to all other players
            const NetAllPlayersInfo& allPlayersInfo = GetPlayersInfo();
            
            // If did not succeed, then nothing to do
            if (!didSucceed) {
                return;
            }
            // Sanity check: If not actually in a lobby currently, then something went wrong earlier so do nothing
            if (!allPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Not actually in a lobby!");
                return;
            }
            
            // Check frequent cases where shouldn't sent player spot mapping
            if (!allPlayersInfo.netLobbyInfo.isLocalPlayerLobbyOwner) { // If not host, do nothing (only host controls the mapping stuff)
                return;
            }
            if (allPlayersInfo.netLobbyInfo.lobbyMemberIds.size() < 2) { // If no other players in lobby, then nothing to do (as no one to send messages to)
                return;
            }
            // IDEA: If no player spot change, then do nothing. (Just to decrease number of ReliableOrdered/TCP messages for bit of speed) [Won't really matter if not using "TCP" anymore)

            const NetPlayerSpotMapping& netPlayerSpotMapping = allPlayersInfo.netLobbyInfo.playerSpotMapping;
            const PlayerIdPerSpot& playerIdPerSpot = netPlayerSpotMapping.GetUnderlyingMapping();
            
            // Additional sanity checks
            if (!allPlayersInfo.netLobbyInfo.playerSpotMapping.IsMappingSet()) {
                mLogger.AddWarnNetLog("Trying to send player spot mapping but it's not setup!");
                return;
            }
            if (netPlayerSpotMapping.GetTotalPlayers() != allPlayersInfo.netLobbyInfo.lobbyMemberIds.size()) {
                mLogger.AddWarnNetLog(
                    "Mismatch in player sizes! Lobby member ids size: " + std::to_string(allPlayersInfo.netLobbyInfo.lobbyMemberIds.size())
                    + ", player spot mapping total players: " + std::to_string(netPlayerSpotMapping.GetTotalPlayers())
                );
                return;
            }
            if (netPlayerSpotMapping.GetTotalPlayers() != playerIdPerSpot.GetSize()) { // Bit more excessive than usual, but these *are* sanity checks
                mLogger.AddWarnNetLog(
                    "Mismatch in player sizes! Underlying mapping size: " + std::to_string(playerIdPerSpot.GetSize())
                    + ", player spot mapping total players: " + std::to_string(netPlayerSpotMapping.GetTotalPlayers())
                );
                return;
            }
            if (PlayerSpotHelpers::IsInvalidTotalPlayers(netPlayerSpotMapping.GetTotalPlayers())) {
                mLogger.AddWarnNetLog(
                    "Total players is out of range somehow! Max player spots: " + std::to_string(PlayerSpotHelpers::kMaxPlayerSpots)
                    + ", total players: " + std::to_string(netPlayerSpotMapping.GetTotalPlayers())
                );
                return;
            }
            
            PlayerSpotMappingMessage messageToSend = {};
            messageToSend.totalPlayers = netPlayerSpotMapping.GetTotalPlayers();
            
            // Convert each id (in order) to "raw" data format that we can send to other players
            for (uint32_t playerSpotIndex = 0; playerSpotIndex < playerIdPerSpot.GetSize(); playerSpotIndex++) {
                const CrossPlatformIdWrapper& playerId = playerIdPerSpot.Get(playerSpotIndex);
                
                // 1. Convert id to string (as underlying id type is a sort of pointer)
                std::string playerIdAsString;
                bool didToStringSucceed = playerId.TryToString(playerIdAsString);
                if (!didToStringSucceed) {
                    mLogger.AddWarnNetLog("Failed to convert an id to string!");
                    return;
                }
                
                // Sanity check length expectations
                if (playerIdAsString.length() > EOS_PRODUCTUSERID_MAX_LENGTH) {
                    mLogger.AddWarnNetLog(
                        "Player id length is greater than expected! Max length without null terminating char: " +
                        std::to_string(EOS_PRODUCTUSERID_MAX_LENGTH) + ", actual id string length: " +
                        std::to_string(playerIdAsString.length())
                    );
                    return;
                }

                // 2. Convert to raw char array (where each one is length EOS_PRODUCTUSERID_MAX_LENGTH)
                char* targetRawArray = messageToSend.rawPlayerSpotMapping[playerSpotIndex];
                for (int i = 0; i < playerIdAsString.length(); i++) { // Don't use strcpy as null terminating character may be in one spot beyond EOS_PRODUCTUSERID_MAX_LENGTH
                    targetRawArray[i] = playerIdAsString.at(i);
                }
                // Add null terminating character if not using full array.
                //      If no null terminating character within the array, then receiver will assume the rest of the
                //      array is actually part of the id (ie, EOS_PRODUCTUSERID_MAX_LENGTH+1 char is assumed to be '\0')
                if (playerIdAsString.length() < EOS_PRODUCTUSERID_MAX_LENGTH) {
                    targetRawArray[playerIdAsString.length()] = '\0';
                }
            }

            // Finally send the message to all peers!
            SendP2PMessageToAllPlayersInMatchLobby(messageToSend, mDefaultPacketReliability);
        }
        void ProcessReceivedPlayerSpotMapping(const CrossPlatformIdWrapper& senderId,
                                              const PlayerSpotMappingMessage& message) {
            const NetAllPlayersInfo& allPlayersInfo = GetPlayersInfo();
            
            // Context sanity checks
            if (!allPlayersInfo.netLobbyInfo.isInLobby) {
                mLogger.AddWarnNetLog("Not actually in a lobby!");
                return;
            }
            if (senderId != allPlayersInfo.netLobbyInfo.lobbyOwner) {
                mLogger.AddWarnNetLog("Message not sent by host, only host is expected to send this message!");
                return;
            }
            // Message sanity checks
            if (PlayerSpotHelpers::IsInvalidTotalPlayers(message.totalPlayers)) {
                mLogger.AddWarnNetLog(
                    "Total players is out of range! Max player spots: " + std::to_string(PlayerSpotHelpers::kMaxPlayerSpots)
                    + ", total players: " + std::to_string(message.totalPlayers)
                );
                return;
            }
            if (message.totalPlayers != allPlayersInfo.netLobbyInfo.lobbyMemberIds.size()) {
                mLogger.AddWarnNetLog(
                    "Mismatch in player sizes! Lobby member ids size: " + std::to_string(allPlayersInfo.netLobbyInfo.lobbyMemberIds.size())
                    + ", player spot mapping total players: " + std::to_string(message.totalPlayers)
                );
            }
            
            // Try to convert raw ids to properly formatted ones
            std::vector<CrossPlatformIdWrapper> allPlayerIdsInOrder;
            allPlayerIdsInOrder.resize(message.totalPlayers);
            for (uint8_t i = 0; i < message.totalPlayers; i++) {
                char safeRawId[EOS_PRODUCTUSERID_MAX_LENGTH + 1];
                const char* targetRawArray = message.rawPlayerSpotMapping[i];
                
                // Mark last character with the null terminating character.
                //      Input array is only EOS_PRODUCTUSERID_MAX_LENGTH, while max actual length is +1 that size for
                //          the last terminating character (ie, in case all chars for id is actually used).
                //      Message only contains EOS_PRODUCTUSERID_MAX_LENGTH per id as no need to send the last extra character.
                //      After all, we'd manually be setting the last character anyways to protect from overflow attacks.
                safeRawId[EOS_PRODUCTUSERID_MAX_LENGTH] = '\0';
                
                // Copy the message's raw id to properly delimited (and thus safe to use) array
                memcpy(safeRawId, targetRawArray, EOS_PRODUCTUSERID_MAX_LENGTH);
                
                // Convert to proper id type
                std::string idAsStandardString = safeRawId;
                CrossPlatformIdWrapper properId = CrossPlatformIdWrapper::FromString(idAsStandardString);

                // Sanity check: Make sure id is in lobby, as otherwise could be any arbitrary value
                if (!allPlayersInfo.netLobbyInfo.IsIdInLobbyMembers(properId)) {
                    mLogger.AddWarnNetLog("Did not find following id in current lobby members: " + idAsStandardString);
                    return;
                }

                // Finally we have the validated id
                allPlayerIdsInOrder[i] = properId; 
            }

            // Set the resulting mapping so now accessible to any relevant code
            mPlayersInfoManager.SetPlayerSpotMappingFromHost(allPlayerIdsInOrder);

            // Notify in case downstream will do any updates based on the new mapping (eg, update lobby UI with correct order)
            mNetSubscribersManager.OnReceivedPlayerSpotMapping();
        }

        // For now, send "game management" messages with TCP-type reliability to reduce any potential issues with message ordering.
        //      Ex: What if start match messages are processed before player spot mapping is received?
        //      Note that this is really a "just-in-case" bandaid and we really should have better mechanisms to actually
        //          prevent these kinds of issues (like acks x unique update ids for player spot mapping messages)
        static constexpr PacketReliability mDefaultPacketReliability = PacketReliability::ReliableOrdered;

        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();
        EOSWrapperSingleton& mEosWrapperSingleton = Singleton<EOSWrapperSingleton>::get();

        bool mIsInitialized = false;
        NetSubscribersManager mNetSubscribersManager = {};

        // TODO: Move connection tracking to players info tracking class
        bool mIsConnectedToOtherPlayer = false;
        PlayerId mConnectedPlayerId = PlayerId(PlayerSpot::Player1);

        NetPlayersInfoManager mPlayersInfoManager = {};
    };
}
