#pragma once

#include <string>
#include <EOS/Include/eos_logging.h>
#include <EOS/Include/eos_init.h>
#include <EOS/Include/eos_auth.h>
#include <EOS/Include/eos_p2p.h>
#include <EOS/Include/eos_sdk.h>

#include "CrossPlatformIdWrapper.h"
#include "EpicAccountIdWrapper.h"
#include "IEOSWrapperManager.h"
#include "PacketReliability.h"
#include "Network/NetworkManagerCallbackTypes.h"
#include "Secrets/NetworkSecrets.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    class EOSWrapperSingleton {
        const std::string TEST_SOCKET_NAME = "TEST_SOCKET";
        
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
        EOSWrapperStatus currentNetStatus = EOSWrapperStatus::NotInitialized;

        NetStatusChangeCallback netStatusChangeCallback = nullptr;
        NetGotSelfIdCallback netGotSelfIdCallback = nullptr;
        IEOSWrapperManager* eosWrapperManager = nullptr;

        EpicAccountIdWrapper loggedInEpicAccountId = {};
        CrossPlatformIdWrapper loggedInCrossPlatformId = {};

        // Various EOS handles
        EOS_HPlatform platformHandle = nullptr;
        EOS_HAuth authHandle = nullptr;
        EOS_HConnect connectHandle = nullptr;

        // Id used with managing incoming connection notifications. Not the same as a socket id or such
        EOS_NotificationId connectionNotificationId = EOS_INVALID_NOTIFICATIONID;

        // TODO: For EOS ids, do we need to copy and free them? Are memory leaks happening when logging in and out?
        // FUTURE: Need pretty complex, thorough login flow to handle all platforms and cases. Need to thoroughly re-read relevant docs
            // Note that may need to even have a refresh auth token workflow as well

        // FUTURE: How to not just willy-nilly accept every connection request and thus protect people's IPs?
            // Perhaps socket name == just the current session/lobby's hidden internal name? That way it's always changing
            // but known to all relevant players AND unknown to outsiders (including viewers of streams)

        // TODO: Full tracking of all open connections. See following doc for all events:
        // https://dev.epicgames.com/docs/services/en-US/GameServices/P2P/index.html?sessionInvalidated=true#closingconnections
            // Also close all existing connections when calling Cleanup
    
    public:
        EOSWrapperSingleton() {}

        ~EOSWrapperSingleton() {
            // Do a true shutdown as applicable
            cleanupState(true);
        }

#pragma region "Lifecycle"

        /// <summary>Initialize based on EOS SDK sample app's FMain::InitPlatform()</summary>
        /// <returns>Returns true if initialization succeeded</returns>
        bool tryInitialize() {
            if (isInitialized()) {
                logger.addInfoNetLog("EWS::Initialize", "NetworkManager is already initialized");
                return false;
            }

            EOS_InitializeOptions sdkOptions = {};
            sdkOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
            sdkOptions.AllocateMemoryFunction = nullptr;
            sdkOptions.ReallocateMemoryFunction = nullptr;
            sdkOptions.ReleaseMemoryFunction = nullptr;
            sdkOptions.ProductName = "DragonJawad's Test Product";
            sdkOptions.ProductVersion = "0.1";
            sdkOptions.Reserved = nullptr;
            sdkOptions.SystemInitializeOptions = nullptr;
            sdkOptions.OverrideThreadAffinity = nullptr;

            EOS_EResult initResult = EOS_Initialize(&sdkOptions);
            if (initResult == EOS_EResult::EOS_AlreadyConfigured) {
                // This case may occur if EOS is init, shutdown, then attempted to be initialized again
                // Why doesn't EOS_Initialize support being called again when the shutdown was called successfully?
                // No idea, just proceed onwards
                // Update: In reality, callback setting and platform creation never succeed if already configured .-.
                logger.addInfoNetLog(
                    "EWS::Initialize",
                    "EOS_Initialize returned EOS_AlreadyConfigured, proceeding with init attempt"
                );
            } else if (initResult != EOS_EResult::EOS_Success) {
                logger.addErrorNetLog(
                    "EWS::Initialize",
                    "EOS SDK failed to init with result: " + std::string(EOS_EResult_ToString(initResult))
                );
                return false;
            }

            logger.addNetLogMessage("EOS SDK initialized, setting logging callback...");
            EOS_EResult setLogCallbackResult = EOS_Logging_SetCallback([](const EOS_LogMessage* message) {
                Singleton<EOSWrapperSingleton>::get().handleEosLogMessage(message);
            });
            if (setLogCallbackResult != EOS_EResult::EOS_Success) {
                logger.addWarnNetLog("EWS::Initialize", "Set logging callback failed");
            } else {
                logger.addInfoNetLog("EWS::Initialize", "Logging callback set");
                EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Verbose);
            }

            if (tryCreatePlatformInstance()) {
                setNetStatus(EOSWrapperStatus::Initialized);
                return true;
            }
            return false;
        }

        /// <param name="forceShutdown">If true, then will actually shutdown. Yes it's kind of dumb that false does nothing atm</param>
        void cleanupState(bool forceShutdown) {
            // FUTURE: Theoretically *should* clean up eosWrapperManager reference, but both are singletons
            //          Thus, theoretically also don't need to clean that up... at least right now
            
            // Clean up callbacks to assure they don't persist between game instances/runs
            netStatusChangeCallback = nullptr;
            netGotSelfIdCallback = nullptr;

            // EOS is super dumb- failing to Init -> Shutdown -> Init again
            // Recommendation from a year ago is even to never shutdown in editor:
            // https://eoshelp.epicgames.com/s/question/0D52L00004Ss2leSAB/platform-initialization-fails-from-2nd-time-on-unless-i-restart-unity?language=en_US
            // (See Francesco reply)
            // Thus, we'll never actually shutdown this instance outside of certain very explicit scenarios
            // (eg, when Singleton is destroyed- albeit at that point, the entire game/program is likely ending anyways)
            if (forceShutdown) {
                if (isInitialized()) {
                    // Assuming don't need to explicitly log out or such when shutting down, so just reset login-related state
                    resetLoginData();

                    // Release platform and "shutdown" SDK as a whole (whatever that does) 
                    EOS_Platform_Release(platformHandle);
                    EOS_Shutdown();
                    platformHandle = nullptr;

                    // Shutdown is supposedly done! (Even if it fails, we won't know right now)
                    // At the very least, we can report that our work here is complete and we are no longer initialized
                    currentNetStatus = EOSWrapperStatus::NotInitialized;
                    logger.addInfoNetLog("EWS::Shutdown", "Shutdown attempt completed");
                }
                else {
                    logger.addInfoNetLog("EWS::Shutdown", "Not initialized");
                }
            }
        }

        // Normal update per frame
        void update() {
            if (platformHandle) {
                EOS_Platform_Tick(platformHandle);
            }

            handleReceivedMessages();
        }

        /// <summary>Login with SDK DevAuth tools based on SDK sample's FAuthentication::Login</summary>
        /// <returns>Returns true if login request is successfully sent</returns>
        bool tryBeginLoginAttempt(const std::string& devAuthName) {
            if (!isInitialized()) {
                logger.addInfoNetLog("EWS::Login", "Not initialized");
                return false;
            }
            if (isBasicLoggedIn()) {
                logger.addInfoNetLog("EWS::Login", "Already logged in");
                return false;
            }

            authHandle = EOS_Platform_GetAuthInterface(platformHandle);
            connectHandle = EOS_Platform_GetConnectInterface(platformHandle);

            // TODO: Setup AddNotifyLoginStatusChanged() (login status callback) if/as necessary
            // See SDK sample FAuthentication::Login's call here towards the beginning
            // Also read SDK's "Status Change Notification" section:
            // https://dev.epicgames.com/docs/services/en-US/EpicAccountServices/AuthInterface/index.html#statuschangenotification

            EOS_Auth_Credentials credentials = {};
            credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;

            EOS_Auth_LoginOptions loginOptions = {};
            // Example memset this to 0 for some reason and IDE suggested initializing like this instead
            loginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
            // Scope flags are arbitrarily chosen based on descriptions and example login code
            // In future, we should further examine the various options and use what's most appropriate
            // (Note that EOS SDK actually goes into decent detail on this, eg user will see all requested perms. Good read!)
            loginOptions.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList
                | EOS_EAuthScopeFlags::EOS_AS_Presence;

            setCredentialsForDevAuthLogin(credentials, devAuthName);

            loginOptions.Credentials = &credentials;
            EOS_Auth_Login(authHandle, &loginOptions, this, [](const EOS_Auth_LoginCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().loginCompleteCallback(data);
            });

            // TODO: Setup AddConnectAuthExpirationNotification() (connect auth expiration callback) if/as necessary
            // Waiting until this is really necessary to truly understand it before implementing

            logger.addInfoNetLog("EWS::Login", "Successfully sent login request");
            return true;
        }

        void logout() {
            if (!isBasicLoggedIn()) {
                logger.addInfoNetLog("EWS::Logout", "Not logged in");
                return;
            }
            if (authHandle == nullptr) {
                logger.addErrorNetLog("EWS::Logout", "Is logged in but somehow no auth handle");
                return;
            }

            logger.addInfoNetLog("EWS::Logout", "Logging out...");

            EOS_Auth_LogoutOptions LogoutOptions;
            LogoutOptions.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
            LogoutOptions.LocalUserId = loggedInEpicAccountId.getAccountId();

            EOS_Auth_Logout(authHandle, &LogoutOptions, nullptr, [](const EOS_Auth_LogoutCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().logoutCompleteCallback(data);
            });
        }

#pragma endregion
#pragma region Callback Registration

        void registerNetStatusChangeListener(const NetStatusChangeCallback& callback) {
            netStatusChangeCallback = callback;

            // Immediately call the callback as a workaround to Singleton being long-living (eg, already being initialized on editor play start)
            netStatusChangeCallback(currentNetStatus);
        }

        void registerNetGotSelfIdListener(const NetGotSelfIdCallback& callback) {
            netGotSelfIdCallback = callback;

            // Not expecting id to already be set in reality, but just in case- if id is somehow set, then call callback immediately
            if (loggedInCrossPlatformId.isValid()) {
                std::string result;
                if (!loggedInCrossPlatformId.tryToString(result)) {
                    logger.addErrorNetLog("EWS::registerNetGotSelfIdListener", "Failed to convert id to string");
                    return;
                }

                callback(result);
            }
        }

        void setWrapperManager(IEOSWrapperManager* iEOSWrapperManager) {
            eosWrapperManager = iEOSWrapperManager;
        }

#pragma endregion
#pragma region Ordinary Connection & Message Sending

        CrossPlatformIdWrapper convertToCrossPlatformIdType(const std::string& targetCrossPlatformId) {
            EOS_ProductUserId formattedTargetCrossPlatformId = EOS_ProductUserId_FromString(targetCrossPlatformId.c_str());
            return CrossPlatformIdWrapper(formattedTargetCrossPlatformId);
        }

        void sendMessage(CrossPlatformIdWrapper targetId, const void* data, uint32_t dataLengthInBytes, PacketReliability packetReliability) {
            if (!isCrossPlatformLoggedIn()) {
                logger.addWarnNetLog("EWS::sendMessage", "Not cross platform logged in");
                return;
            }
            if (!targetId.isValid()) {
                logger.addWarnNetLog("EWS::sendMessage", "Target cross platform id is invalid");
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(platformHandle);

            EOS_P2P_SocketId socketId;
            socketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
            strncpy_s(socketId.SocketName, "CHAT", 5);

            EOS_P2P_SendPacketOptions options;
            options.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
            options.LocalUserId = loggedInCrossPlatformId.getAccountId();
            options.RemoteUserId = targetId.getAccountId();
            options.SocketId = &socketId;
            options.bAllowDelayedDelivery = EOS_TRUE; // PLACEHOLDER: Set to false once separately setting up connections
            options.Channel = 0;
            options.Reliability = convertPacketReliability(packetReliability);

            options.DataLengthBytes = dataLengthInBytes;
            options.Data = data;

            EOS_EResult result = EOS_P2P_SendPacket(p2pHandle, &options);
            if (result == EOS_EResult::EOS_Success) {
                logger.addInfoNetLog(
                    "EWS::TestSendMessage",
                    "Successfully attempted to send message"
                );
            }
            if (result != EOS_EResult::EOS_Success) {
                logger.addErrorNetLog(
                    "EWS::TestSendMessage",
                    "Failed to send message");
            }
        }
        
#pragma endregion 
#pragma region Debug/Testing Methods

        void testSendMessage(const std::string& targetCrossPlatformId, const std::string& message) {
            if (targetCrossPlatformId.empty()) {
                logger.addWarnNetLog("EWS::TestSendMessage", "Empty targetCrossPlatformId");
                return;
            }
            if (message.empty()) {
                logger.addWarnNetLog("EWS::TestSendMessage", "Empty message, nothing to send");
                return;
            }

            sendMessage(
                CrossPlatformIdWrapper::fromString(targetCrossPlatformId),
                message.data(),
                static_cast<uint32_t>(message.size()),
                PacketReliability::ReliableOrdered
            );            
        }

        void printLoggedInId() {
            if (!isBasicLoggedIn()) {
                logger.addInfoNetLog("EWS::PrintLoggedInId", "Not logged in");
                return;
            }

            std::string idAsString;
            if (!loggedInEpicAccountId.tryToString(idAsString)) {
                logger.addErrorNetLog("EWS::PrintLoggedInId", "Failed to convert id to string");
                return;
            }

            logger.addInfoNetLog("EWS::PrintLoggedInId", "Id: " + idAsString);
        }

        void printLoggedInCrossPlatformId() {
            if (!isCrossPlatformLoggedIn()) {
                logger.addInfoNetLog("EWS::PrintLoggedInCrossPlatformId", "Not logged in");
                return;
            }

            std::string idAsString;
            if (!loggedInCrossPlatformId.tryToString(idAsString)) {
                logger.addErrorNetLog("EWS::PrintLoggedInCrossPlatformId", "Failed to convert id to string");
                return;
            }

            logger.addInfoNetLog("EWS::PrintLoggedInCrossPlatformId", "Id: " + idAsString);
        }

#pragma endregion
#pragma region EOS Callbacks

        void handleEosLogMessage(const EOS_LogMessage* message) {
            std::string identifier = "[EOS SDK] " + std::string(message->Category);

            switch (message->Level) {
            case EOS_ELogLevel::EOS_LOG_Warning:
                logger.addWarnNetLog(identifier, message->Message, NetLogCategory::EosSdk);
                break;

            case EOS_ELogLevel::EOS_LOG_Error:
            case EOS_ELogLevel::EOS_LOG_Fatal:
                logger.addErrorNetLog(identifier, message->Message, NetLogCategory::EosSdk);
                break;

            default:
                logger.addInfoNetLog(identifier, message->Message, NetLogCategory::EosSdk);
            }
        }

        // Based on SDK sample's FAuthentication::LoginCompleteCallbackFn
        void loginCompleteCallback(const EOS_Auth_LoginCallbackInfo* data) {
            if (data == nullptr) {
                logger.addErrorNetLog("EWS::LoginCompleteCallback", "Input data is unexpectedly null");
                return;
            }

            if (data->ResultCode == EOS_EResult::EOS_Success) {
                onSuccessfulEpicLogin(data->LocalUserId);
            }
            else if (data->ResultCode == EOS_EResult::EOS_Auth_PinGrantCode) {
                logger.addWarnNetLog("EWS::LoginCompleteCallback", "Waiting for PIN grant...?");
            }
            else if (data->ResultCode == EOS_EResult::EOS_Auth_MFARequired) {
                logger.addWarnNetLog(
                    "EWS::LoginCompleteCallback",
                    "MFA Code needs to be entered before logging in"
                );

                // PLACEHOLDER: Callback or such to handle this case
                // See SDK sample relevant code (event type UserLoginRequiresMFA). Eg:
                // FGameEvent Event(EGameEventType::UserLoginRequiresMFA, Data->LocalUserId);
            }
            else if (data->ResultCode == EOS_EResult::EOS_InvalidUser) {
                if (data->ContinuanceToken != nullptr) {
                    logger.addInfoNetLog("EWS::LoginCompleteCallback", "Login failed, external account not found");

                    // PLACEHOLDER: Check sample's FAuthentication::LoginCompleteCallbackFn for relevant code here
                    // Something something try to continue login or something...?
                    
                    // See following section in docs for more info:
                    // https://dev.epicgames.com/docs/services/en-US/EpicAccountServices/AuthInterface/index.html#externalaccountauthentication
                } else {
                    logger.addErrorNetLog("EWS::LoginCompleteCallback", "Continuation Token is invalid");
                }
            }
            else if (data->ResultCode == EOS_EResult::EOS_Auth_AccountFeatureRestricted) {
                if (data->AccountFeatureRestrictedInfo) {
                    std::string verificationURI = data->AccountFeatureRestrictedInfo->VerificationURI;
                    logger.addErrorNetLog(
                        "EWS::LoginCompleteCallback",
                        "Login failed, account is restricted. User must visit URI: " + verificationURI
                    );
                } else {
                    logger.addErrorNetLog(
                        "EWS::LoginCompleteCallback",
                        "Login failed, account is restricted. VerificationURI is invalid!"
                    );
                }
            }
            else if (EOS_EResult_IsOperationComplete(data->ResultCode)) {
                logger.addErrorNetLog(
                    "EWS::LoginCompleteCallback",
                    "Login Failed - Error Code: " + toString(data->ResultCode)
                );
            }
            else {
                logger.addWarnNetLog("EWS::LoginCompleteCallback", "Hit final else statement unexpectedly");
            }
        }

        void crossPlatformLoginCompleteCallback(const EOS_Connect_LoginCallbackInfo* data) {
            if (data == nullptr) {
                logger.addErrorNetLog("EWS::CrossPlatformLoginCompleteCallback", "Data is nullptr");
                return;
            }

            if (data->ResultCode == EOS_EResult::EOS_Success) {
                onSuccessfulCrossPlatformLogin(data->LocalUserId);
            }
            else {
                logger.addErrorNetLog(
                    "EWS::CrossPlatformLoginCompleteCallback",
                    "Cross plat login failed with result: " + toString(data->ResultCode));
            }
        }

        void logoutCompleteCallback(const EOS_Auth_LogoutCallbackInfo* data) {
            if (data == nullptr) {
                logger.addErrorNetLog("EWS::LogoutCompleteCallback", "Input data is unexpectedly null");
                return;
            }

            if (data->ResultCode == EOS_EResult::EOS_Success) {
                logger.addInfoNetLog("EWS::LogoutCompleteCallback", "Logged out successfully");

                // Note that logging out event game-wise can (and likely should be) handled on the LoginStatusChanged callback side
            }
            else {
                logger.addErrorNetLog(
                    "EWS::LogoutCompleteCallback",
                    "Logout Failed - Error Code: " + toString(data->ResultCode)
                );
            }

            // Arbitrarily assuming that not logged in regardless of what result occurred. Thus resetting related data
            // (Note that in reality there COULD be race conditions with these callbacks depending on how the SDK is implemented)
            // (Thus this may burn us in the future)
            unsubscribeFromConnectionRequests();
            resetLoginData();
        }

        // Based on SDK sample's FP2PNAT::OnIncomingConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* Data)
        void onIncomingConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* data) {
            if (data == nullptr) {
                logger.addErrorNetLog("EWS::OnIncomingConnectionRequest", "Input data is unexpectedly null");
                return;
            }

            if (!isCrossPlatformLoggedIn()) {
                logger.addErrorNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "Somehow receiving connection requests without being cross platform logged in. Perhaps outdated expectation?");
                return;
            }

            std::string socketName = data->SocketId->SocketName;
            if (socketName != "CHAT") {
                logger.addWarnNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "Unexpected socket name, ignoring. Incoming name: " + socketName);
                return;
            }

            // Accepting connection based on SDK sample's FP2PNAT::OnIncomingConnectionRequest
            
            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(platformHandle);
            EOS_P2P_AcceptConnectionOptions options;
            options.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
            options.LocalUserId = loggedInCrossPlatformId.getAccountId();
            options.RemoteUserId = data->RemoteUserId;

            EOS_P2P_SocketId socketId;
            socketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
            // strncpy_s(socketId.SocketName, TEST_SOCKET_NAME.c_str(), TEST_SOCKET_NAME.length() + 1);
            strncpy_s(socketId.SocketName, "CHAT", 5);
            options.SocketId = &socketId;

            EOS_EResult result = EOS_P2P_AcceptConnection(p2pHandle, &options);
            if (result == EOS_EResult::EOS_Success) {
                CrossPlatformIdWrapper wrappedRemoteId = CrossPlatformIdWrapper(data->RemoteUserId);
                std::string remoteIdAsString;
                if (!wrappedRemoteId.tryToString(remoteIdAsString)) {
                    logger.addWarnNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "Could not convert remote user id to string"
                    );
                    return;
                }
                
                logger.addInfoNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "Successfully accepted connection from: " + remoteIdAsString
                );
            }
            else {
                logger.addErrorNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "EOS_P2P_AcceptConnection failed with result: " + toString(result)
                );
            }
        }

#pragma endregion

    private:
        /// <returns>True if EOS Platform successfully initialized</returns>
        bool tryCreatePlatformInstance() {
            EOS_Platform_Options platformOptions = {};
            platformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;

            platformOptions.ProductId = NetworkSecrets::ProductId;
            platformOptions.SandboxId = NetworkSecrets::SandboxId;
            platformOptions.DeploymentId = NetworkSecrets::DeploymentId;
            platformOptions.ClientCredentials.ClientId = NetworkSecrets::ClientCredentialsId;
            platformOptions.ClientCredentials.ClientSecret = NetworkSecrets::ClientCredentialsSecret;

            platformOptions.Flags = 0;
            // For now, don't use any flags for testing consistency with full default features
            // PlatformOptions.Flags = EOS_PF_LOADING_IN_EDITOR; // In future, need to understand the use case of each flag and test appropriately

            // Unused features
            platformOptions.TickBudgetInMilliseconds = 0; // NOTE: Set this to some reasonable value in future
            platformOptions.bIsServer = false;
            platformOptions.EncryptionKey = nullptr;
            platformOptions.OverrideCountryCode = nullptr;
            platformOptions.OverrideLocaleCode = nullptr;
            platformOptions.CacheDirectory = nullptr;
            platformOptions.RTCOptions = nullptr;

            platformHandle = EOS_Platform_Create(&platformOptions);
            if (platformHandle == nullptr) {
                logger.addErrorNetLog("EWS::createPlatformInstance", "EOS Platform failed to init");
                return false;
            }

            logger.addInfoNetLog("EWS::createPlatformInstance", "EOS Platform successfully initialized!");
            return true;
        }

        /// <param name="credentials">Credentials reference mate</param>
        /// <param name="accountName">Called "credentials" in sample app, this is the name of the account setup in DevAuthTool</param>
        void setCredentialsForDevAuthLogin(EOS_Auth_Credentials& credentials, const std::string& accountName) {
            credentials.Id = "localhost:12345"; // Hardcoding expectation that tool server locally running on port 12345
            credentials.Token = accountName.c_str();
            credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
        }

        void onSuccessfulEpicLogin(const EOS_EpicAccountId resultUserId) {
            loggedInEpicAccountId = EpicAccountIdWrapper(resultUserId);

            std::string userIdAsString;
            if (!loggedInEpicAccountId.tryToString(userIdAsString)) {
               logger.addErrorNetLog("EWS::onSuccessfulLogin", "Could not convert user id to string");
            }
            logger.addInfoNetLog(
                "EWS::onSuccessfulLogin",
                "Login Complete - User ID: " + userIdAsString
            );
            
            startCrossPlatformLogin();
        }

        void onSuccessfulCrossPlatformLogin(const EOS_ProductUserId resultCrossPlatformId) {
            loggedInCrossPlatformId = CrossPlatformIdWrapper(resultCrossPlatformId);
            setNetStatus(EOSWrapperStatus::LoggedIn);

            std::string userIdAsString;
            if (!loggedInCrossPlatformId.tryToString(userIdAsString)) {
                logger.addErrorNetLog(
                    "EWS::onSuccessfulCrossPlatformLogin",
                    "Could not convert cross platform id to string");
            }
            logger.addInfoNetLog(
                "EWS::onSuccessfulCrossPlatformLogin",
                "Login Complete - Cross Platform User ID: " + userIdAsString
            );

            if (netGotSelfIdCallback) {
                netGotSelfIdCallback(userIdAsString);
            }
            if (eosWrapperManager) {
                eosWrapperManager->onLoginSuccess(loggedInCrossPlatformId);
            }

            subscribeToConnectionRequests();
        }

        // Based on SDK sample's FP2PNAT::HandleReceivedMessages
        void handleReceivedMessages() {
            if (!isCrossPlatformLoggedIn()) {
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(platformHandle);

            EOS_P2P_ReceivePacketOptions options;
            options.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
            options.LocalUserId = loggedInCrossPlatformId.getAccountId();
            options.MaxDataSizeBytes = 4096; // Very future optimization: Decrease to around the expected max message size
            options.RequestedChannel = nullptr;

            EOS_P2P_SocketId socketId;
            socketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
            uint8_t channel = 0;

            std::vector<char> messageData;
            messageData.resize(options.MaxDataSizeBytes); // Optimization: Initialize to this size already?
            uint32_t bytesWritten = 0;

            EOS_ProductUserId peerId;
            EOS_EResult result = EOS_P2P_ReceivePacket(p2pHandle, &options, &peerId, &socketId, &channel,
                messageData.data(), &bytesWritten);
            if (result == EOS_EResult::EOS_NotFound)
            {
                // no more packets
            }
            else if (result == EOS_EResult::EOS_Success)
            {
                if (eosWrapperManager) {
                    eosWrapperManager->onMessageReceived(CrossPlatformIdWrapper(peerId), messageData);
                }
            }
            else
            {
                logger.addErrorNetLog(
                    "EWS::handleReceivedMessages",
                    "EOS_P2P_ReceivePacket failed with result: " + toString(result)
                );
            }
        }

        void resetLoginData() {
            connectHandle = nullptr;
            authHandle = nullptr;

            loggedInCrossPlatformId = {};
            loggedInEpicAccountId = {};
        }

        // Based on SDK sample's FAuthentication::ConnectLogin
        void startCrossPlatformLogin() {
            if (authHandle == nullptr) {
                logger.addErrorNetLog("EWS::startCrossPlatformLogin", "AuthHandle null");
                return;
            }
            if (connectHandle == nullptr) {
                logger.addErrorNetLog("EWS::startCrossPlatformLogin", "ConnectHandle null");
                return;
            }
            
            EOS_Auth_Token* userAuthToken = nullptr;
            
            EOS_Auth_CopyUserAuthTokenOptions copyTokenOptions = { 0 };
            copyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;
            EOS_EResult result = EOS_Auth_CopyUserAuthToken(authHandle, &copyTokenOptions, loggedInEpicAccountId.getAccountId(), &userAuthToken);
            
            if (result == EOS_EResult::EOS_Success) {
                EOS_Connect_Credentials credentials;
                credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
                credentials.Token = userAuthToken->AccessToken;
                credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;

                EOS_Connect_LoginOptions options = { 0 };
                options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
                options.Credentials = &credentials;
                options.UserLoginInfo = nullptr;

                EOS_Connect_Login(connectHandle, &options, nullptr, [](const EOS_Connect_LoginCallbackInfo* data) {
                    Singleton<EOSWrapperSingleton>::get().crossPlatformLoginCompleteCallback(data);
                });
                EOS_Auth_Token_Release(userAuthToken);
            }
            else {
                logger.addErrorNetLog(
                    "EWS::startCrossPlatformLogin",
                    "EOS_Auth_CopyUserAuthToken failed with result: " + toString(result)
                );
            }
        }

        void subscribeToConnectionRequests() {
            // Safety sanity checks
            if (!isCrossPlatformLoggedIn()) {
                logger.addErrorNetLog(
                    "EWS::subscribeToConnectionRequests",
                    "Not cross platform logged in"
                );
                return;
            }
            if (isSubscribedToConnectionRequests()) {
                logger.addErrorNetLog(
                    "EWS::subscribeToConnectionRequests",
                    "Already subscribed to connection requests"
                );
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(platformHandle);

            EOS_P2P_SocketId socketId;
            socketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
            // strncpy_s(socketId.SocketName, TEST_SOCKET_NAME.c_str(), TEST_SOCKET_NAME.size() + 1);
            strncpy_s(socketId.SocketName, "CHAT", 5); // TODO: Why didn't above custom name setting work?

            EOS_P2P_AddNotifyPeerConnectionRequestOptions options;
            options.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
            options.LocalUserId = loggedInCrossPlatformId.getAccountId();
            options.SocketId = &socketId;

            connectionNotificationId = EOS_P2P_AddNotifyPeerConnectionRequest(p2pHandle, &options, nullptr,
                [](const EOS_P2P_OnIncomingConnectionRequestInfo* data) {
                    Singleton<EOSWrapperSingleton>::get().onIncomingConnectionRequest(data);
                });

            if (isSubscribedToConnectionRequests()) {
                logger.addInfoNetLog(
                    "EWS::subscribeToConnectionRequests",
                    "Successfully subscribed"
                );
            }
            else {
                logger.addErrorNetLog(
                    "EWS::subscribeToConnectionRequests",
                    "Failed to subscribe, bad notification id returned"
                );
            }
        }

        void unsubscribeFromConnectionRequests() {
            if (!isSubscribedToConnectionRequests()) {
                logger.addInfoNetLog("EWS::unsubscribeFromConnectionRequests", "Not subscribed already");
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(platformHandle);
            EOS_P2P_RemoveNotifyPeerConnectionRequest(p2pHandle, connectionNotificationId);
            
            connectionNotificationId = EOS_INVALID_NOTIFICATIONID;
        }

#pragma region Status Helpers
        
        void setNetStatus(EOSWrapperStatus newStatus) {
            currentNetStatus = newStatus;

            if (netStatusChangeCallback) {
                netStatusChangeCallback(currentNetStatus);
            }
        }

        bool isInitialized() {
            return currentNetStatus != EOSWrapperStatus::NotInitialized;
        }
        
        bool isBasicLoggedIn() {
            return loggedInEpicAccountId.isValid();
        }

        bool isCrossPlatformLoggedIn() {
            return loggedInCrossPlatformId.isValid();
        }

        bool isSubscribedToConnectionRequests() {
            return connectionNotificationId != EOS_INVALID_NOTIFICATIONID;
        }

#pragma endregion 
#pragma region EOS SDK type helpers

        std::string toString(const EOS_EResult& result) {
            return EOS_EResult_ToString(result);
        }

        EOS_EPacketReliability convertPacketReliability(PacketReliability inPacketReliability) {
            switch (inPacketReliability) {
                case PacketReliability::UnreliableUnordered:
                    return EOS_EPacketReliability::EOS_PR_UnreliableUnordered;
                case PacketReliability::ReliableUnordered:
                    return EOS_EPacketReliability::EOS_PR_ReliableUnordered;
                case PacketReliability::ReliableOrdered:
                    return EOS_EPacketReliability::EOS_PR_ReliableOrdered;
                default:
                    logger.logWarnMessage(
                        "EWS::convertPacketReliability",
                        "Unexpected PacketReliability value, perhaps missing case?"
                    );
                return EOS_EPacketReliability::EOS_PR_ReliableOrdered;
            }
        }

#pragma endregion
    };
}
