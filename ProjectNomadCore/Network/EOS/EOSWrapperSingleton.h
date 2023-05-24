#pragma once

#include <string>
#include <EOS/Include/eos_logging.h>
#include <EOS/Include/eos_init.h>
#include <EOS/Include/eos_auth.h>
#include <EOS/Include/eos_p2p.h>
#include <EOS/Include/eos_sdk.h>
#include <EOS/Include/eos_lobby.h>
#include <EOS/Include/eos_ui.h>

#include "EOSHelpers.h"
#include "IEOSWrapperManager.h"
#include "Model/CrossPlatformIdWrapper.h"
#include "Model/EpicAccountIdWrapper.h"
#include "Model/NetLobbySlot.h"
#include "Model/PacketReliability.h"
#include "Model/EOSLobbyTracking.h"
#include "Secrets/NetworkSecrets.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    /**
    * This class encapsulates all interactions directly with the EOS SDK.
    *
    * "Why Singleton": This is originally a singleton as can't bind "this" to EOS callbacks. (Easy to test and see the error)
    *       There are other alternatives, sch as how UE's OnlineSubsystemEOS plugin uses some fancy encapsulated-static
    *       magic (see TEOSCallback in OnlineSubsystemEOSTypes.h).
    *       However, we have the Singleton setup and thus might as well use it.
    *       Also singleton pattern feels cleaner cuz less magic... but maybe that's the lazy talking >.>
    **/
    class EOSWrapperSingleton {
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
            CleanupState(true);
        }

        #pragma region "Lifecycle"
        /// <summary>Initialize based on EOS SDK sample app's FMain::InitPlatform()</summary>
        /// <returns>Returns true if initialization succeeded</returns>
        bool TryInitialize() {
            if (IsInitialized()) {
                mLogger.AddWarnNetLog("NetworkManager is already initialized");
                return false;
            }

            EOS_InitializeOptions sdkOptions = {};
            sdkOptions.ApiVersion = EOS_INITIALIZE_API_LATEST;
            sdkOptions.AllocateMemoryFunction = nullptr;
            sdkOptions.ReallocateMemoryFunction = nullptr;
            sdkOptions.ReleaseMemoryFunction = nullptr;
            sdkOptions.ProductName = "Nomad's Rise";
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
                mLogger.AddInfoNetLog(
                    "EOS_Initialize returned EOS_AlreadyConfigured, proceeding with init attempt"
                );
            } else if (initResult != EOS_EResult::EOS_Success) {
                mLogger.AddErrorNetLog(
                    "EOS SDK failed to init with result: " + std::string(EOS_EResult_ToString(initResult))
                );
                return false;
            }

            mLogger.AddInfoNetLog("EOS SDK initialized, setting logging callback...");
            EOS_EResult setLogCallbackResult = EOS_Logging_SetCallback([](const EOS_LogMessage* message) {
                Singleton<EOSWrapperSingleton>::get().HandleEosLogMessage(message);
            });
            if (setLogCallbackResult != EOS_EResult::EOS_Success) {
                mLogger.AddWarnNetLog("Set logging callback failed");
                return false;
            } 

            mLogger.AddInfoNetLog("Logging callback set");
            EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Verbose);

            if (TryCreatePlatformInstance()) {
                mIsInitialized = true;
                return true;
            }
            
            return false; // Did not successfully create the platform instance
        }

        /// <param name="forceShutdown">If true, then will actually shutdown. Yes it's kind of dumb that false does nothing atm</param>
        void CleanupState(bool forceShutdown) {
            if (!IsInitialized()) { // Eg, if EOS SDK was intentionally never initialized
                return; // Intention: Save potential warning messages from filling up the log due to nothing to clean up
            }
            
            // TODO: Make sure not missing any other clean up (but see forceShutdown cases too)
            //       Make sure to clear up all lobbies + sessions at least so not orphaned!

            if (IsLoggedInAtAll()) {
                ResetLoginData();
            }

            // Clear cached user info juuust in case "anything" changes, like user display names. Very minor
            mPlayersInfoTracking = {};
            
            // Clean up callbacks to assure they don't persist between game instances/runs
            mEosWrapperManager = nullptr; // Assume no longer safe reference (eg, perhaps caller has been destroyed already)
            
            // EOS is super dumb- failing to Init -> Shutdown -> Init again
            // Recommendation from a year ago is even to never shutdown in editor:
            // https://eoshelp.epicgames.com/s/question/0D52L00004Ss2leSAB/platform-initialization-fails-from-2nd-time-on-unless-i-restart-unity?language=en_US
            // (See Francesco reply)
            // Thus, we'll never actually shutdown this instance outside of certain very explicit scenarios
            // (eg, when Singleton is destroyed- albeit at that point, the entire game/program is likely ending anyways)
            if (forceShutdown) {
                if (IsInitialized()) {
                    // Release platform and "shutdown" SDK as a whole (whatever that does) 
                    EOS_Platform_Release(mPlatformHandle);
                    EOS_Shutdown();
                    mPlatformHandle = nullptr;

                    // Shutdown is supposedly done! (Even if it fails, we won't know right now)
                    // At the very least, we can report that our work here is complete and we are no longer initialized
                    mIsInitialized = false;
                    mLogger.AddInfoNetLog("Shutdown attempt completed");
                }
                else {
                    mLogger.AddInfoNetLog("Not initialized");
                }
            }
        }

        // Normal update per frame
        void Tick() {
            if (!IsInitialized()) {
                return;
            }
            
            EOS_Platform_Tick(mPlatformHandle);
            HandleReceivedMessages();
            
            // As a workaround to circular dependencies and EOS SDK requiring singleton/static callbacks, we'll check
            //      and send "user info" requests ourselves.
            HandlePendingUserInfoRequests();
        }

        bool BeginLoginAttemptViaAccountPortal() {
            // No need for sanity checks as BeginLoginAttempt() will do them as well. Not doing anything unsafe before then

            EOS_Auth_Credentials credentials = {};
            credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
            SetCredentialsForAccountPortalLogin(credentials);

            return BeginLoginAttempt(credentials);
        }

        /// <summary>Login with SDK DevAuth tools based on SDK sample's FAuthentication::Login</summary>
        /// <returns>Returns true if login request is successfully sent</returns>
        bool BeginLoginAttemptViaDevAuthTool(const std::string& devAuthName, const std::string& ipAndPort) {
            // No need for sanity checks as BeginLoginAttempt() will do them as well. Not doing anything unsafe before then

            EOS_Auth_Credentials credentials = {};
            credentials.ApiVersion = EOS_AUTH_CREDENTIALS_API_LATEST;
            SetCredentialsForDevAuthLogin(credentials, devAuthName, ipAndPort);

            return BeginLoginAttempt(credentials);
        }

        bool BeginLogout() {
            if (!IsBasicLoggedIn()) {
                mLogger.AddWarnNetLog("Not logged in");
                return false;
            }
            if (mAuthHandle == nullptr) {
                mLogger.AddErrorNetLog("Is logged in but somehow no auth handle");
                return false;
            }

            mLogger.AddInfoNetLog("Logging out...");

            EOS_Auth_LogoutOptions LogoutOptions;
            LogoutOptions.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
            LogoutOptions.LocalUserId = mLoggedInEpicAccountId.GetAccountId();

            EOS_Auth_Logout(mAuthHandle, &LogoutOptions, nullptr, [](const EOS_Auth_LogoutCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnLogoutComplete(data);
            });
            return true;
        }
        #pragma endregion

        void SetWrapperManager(IEOSWrapperManager* iEOSWrapperManager) {
            mEosWrapperManager = iEOSWrapperManager;
        }

        void ShowFriendsUI() {
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Not initialized");
                return;
            }
            if (!IsLoggedInAtAll()) {
                mLogger.AddWarnNetLog("Not logged in yet");
                return;
            }
            if (mUIHandle == nullptr) {
                mLogger.AddErrorNetLog("mUIHandle null somehow");
                return;
            }

            // Based on UE OnlineSubsystemEOS plugin's UserManagerEOS::ShowFriendsUI
            EOS_UI_ShowFriendsOptions options = {};
            options.ApiVersion = EOS_UI_SHOWFRIENDS_API_LATEST;
            options.LocalUserId = mLoggedInEpicAccountId.GetAccountId();

            auto passthroughCallback = [](const EOS_UI_ShowFriendsCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnShowFriendsResult(data); // See 
            };
            EOS_UI_ShowFriends(mUIHandle, &options, nullptr, passthroughCallback);
        }

        bool BeginCreateMainMatchLobby() {
            return BeginCreateLobby(NetLobbySlot::MainMatchLobby);
        }
        bool BeginLeaveMainGameLobby() {
            // TODO: Clean up (close) relevant p2p connections
            return BeginLeaveLobby(NetLobbySlot::MainMatchLobby);
        }

        bool SendP2PMessage(const CrossPlatformIdWrapper& targetId,
                            const void* data,
                            uint32_t dataLengthInBytes,
                            PacketReliability packetReliability) {
            if (!IsFullyLoggedIn()) {
                mLogger.AddWarnNetLog("Not cross platform logged in");
                return false;
            }
            if (!targetId.IsValid()) {
                mLogger.AddWarnNetLog("Target cross platform id is invalid");
                return false;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);

            EOS_P2P_SocketId socketId;
            socketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
            strncpy_s(socketId.SocketName, "CHAT", 5); //  TODO: Socket input

            EOS_P2P_SendPacketOptions options;
            options.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
            options.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
            options.RemoteUserId = targetId.GetAccountId();
            options.SocketId = &socketId;
            options.bAllowDelayedDelivery = EOS_TRUE; // PLACEHOLDER: Set to false once separately setting up connections
            options.Channel = 0;
            options.Reliability = EOSHelpers::ConvertPacketReliability(mLogger, packetReliability);
            options.bDisableAutoAcceptConnection = EOS_FALSE; // This seems to default to true as of 2023.05.05

            options.DataLengthBytes = dataLengthInBytes;
            options.Data = data;

            EOS_EResult resultCode = EOS_P2P_SendPacket(p2pHandle, &options);
            if (resultCode != EOS_EResult::EOS_Success) {
                // Use error as failing to queue up message is unexpected
                // FUTURE: If target "refuses" or closes connection, would we fail at this point?
                mLogger.AddErrorNetLog(
                    "Failed to send message with result code: " + EOSHelpers::ResultCodeToString(resultCode)
                );
                return false;
            }
            
            return true; // Message successfully queued up to send
        }

        #pragma region Debug/Testing Methods
        void TestSendMessage(const std::string& targetCrossPlatformId, const std::string& message) {
            if (targetCrossPlatformId.empty()) {
                mLogger.AddWarnNetLog("Empty targetCrossPlatformId");
                return;
            }
            if (message.empty()) {
                mLogger.AddWarnNetLog("Empty message, nothing to send");
                return;
            }

            SendP2PMessage(
                CrossPlatformIdWrapper::FromString(targetCrossPlatformId),
                message.data(),
                static_cast<uint32_t>(message.size()),
                PacketReliability::ReliableOrdered
            );
        }

        void PrintLoggedInId() {
            if (!IsBasicLoggedIn()) {
                mLogger.AddWarnNetLog("Not logged in");
                return;
            }

            std::string idAsString;
            if (!mLoggedInEpicAccountId.TryToString(idAsString)) {
                mLogger.AddErrorNetLog("Failed to convert id to string");
                return;
            }

            mLogger.AddInfoNetLog("Id: " + idAsString);
        }

        void PrintLoggedInCrossPlatformId() {
            if (!IsCrossPlatformLoggedIn()) {
                mLogger.AddWarnNetLog("Not logged in");
                return;
            }

            std::string idAsString;
            if (!mLoggedInCrossPlatformId.TryToString(idAsString)) {
                mLogger.AddErrorNetLog("Failed to convert id to string");
                return;
            }

            mLogger.AddInfoNetLog("Id: " + idAsString);
        }
        #pragma endregion

        #pragma region EOS Callbacks
        void HandleEosLogMessage(const EOS_LogMessage* message) {
            std::string identifier = "[EOS SDK] " + std::string(message->Category);

            switch (message->Level) {
            case EOS_ELogLevel::EOS_LOG_Warning:
                mLogger.AddWarnNetLog(identifier, message->Message, NetLogCategory::EosSdk);
                break;

            case EOS_ELogLevel::EOS_LOG_Error:
            case EOS_ELogLevel::EOS_LOG_Fatal:
                mLogger.AddErrorNetLog(identifier, message->Message, NetLogCategory::EosSdk);
                break;

            default:
                mLogger.AddInfoNetLog(identifier, message->Message, NetLogCategory::EosSdk);
            }
        }

        // Based on SDK sample's FAuthentication::LoginCompleteCallbackFn
        void OnLoginComplete(const EOS_Auth_LoginCallbackInfo* data) {
            if (data == nullptr) {
                mLogger.AddErrorNetLog("Input data is unexpectedly null");
                return;
            }

            if (data->ResultCode == EOS_EResult::EOS_Success) {
                OnSuccessfulEpicLogin(data->LocalUserId);
            }
            else if (data->ResultCode == EOS_EResult::EOS_Auth_PinGrantCode) {
                mLogger.AddWarnNetLog("Waiting for PIN grant...?");
            }
            else if (data->ResultCode == EOS_EResult::EOS_Auth_MFARequired) {
                mLogger.AddWarnNetLog("MFA Code needs to be entered before logging in");

                // PLACEHOLDER: Callback or such to handle this case
                // See SDK sample relevant code (event type UserLoginRequiresMFA). Eg:
                // FGameEvent Event(EGameEventType::UserLoginRequiresMFA, Data->LocalUserId);
            }
            else if (data->ResultCode == EOS_EResult::EOS_InvalidUser) {
                if (data->ContinuanceToken != nullptr) {
                    mLogger.AddWarnNetLog("Login failed, external account not found");

                    // PLACEHOLDER: Check sample's FAuthentication::LoginCompleteCallbackFn for relevant code here
                    // Something something try to continue login or something...?
                    
                    // See following section in docs for more info:
                    // https://dev.epicgames.com/docs/services/en-US/EpicAccountServices/AuthInterface/index.html#externalaccountauthentication
                } else {
                    mLogger.AddErrorNetLog("Continuation Token is invalid");
                }
            }
            else if (data->ResultCode == EOS_EResult::EOS_Auth_AccountFeatureRestricted) {
                if (data->AccountFeatureRestrictedInfo) {
                    std::string verificationURI = data->AccountFeatureRestrictedInfo->VerificationURI;
                    mLogger.AddErrorNetLog("Login failed, account is restricted. User must visit URI: " + verificationURI);
                } else {
                    mLogger.AddErrorNetLog("Login failed, account is restricted. VerificationURI is invalid!");
                }
            }
            else if (EOS_EResult_IsOperationComplete(data->ResultCode)) {
                mLogger.AddErrorNetLog("Login Failed - Error Code: " + EOSHelpers::ResultCodeToString(data->ResultCode));
            }
            else {
                mLogger.AddWarnNetLog("Hit final else statement unexpectedly");
            }
        }
        void OnCrossPlatformLoginComplete(const EOS_Connect_LoginCallbackInfo* data) {
            if (data == nullptr) {
                mLogger.AddErrorNetLog("Data is nullptr");
                return;
            }

            if (data->ResultCode == EOS_EResult::EOS_Success) {
                OnSuccessfulCrossPlatformLogin(data->LocalUserId);
            }
            else {
                mLogger.AddErrorNetLog("Cross plat login failed with result: " + EOSHelpers::ResultCodeToString(data->ResultCode));
            }
        }
        void OnLogoutComplete(const EOS_Auth_LogoutCallbackInfo* data) {
            if (data == nullptr) {
                mLogger.AddErrorNetLog("Input data is unexpectedly null");
                return;
            }

            if (data->ResultCode == EOS_EResult::EOS_Success) {
                mLogger.AddInfoNetLog("Logged out successfully");
                if (mEosWrapperManager) {
                    mEosWrapperManager->OnLogoutSuccess();
                }

                // Note that logging out event game-wise can (and likely should be) handled on the LoginStatusChanged callback side
            }
            else {
                mLogger.AddErrorNetLog("Logout Failed - Error Code: " + EOSHelpers::ResultCodeToString(data->ResultCode));
            }

            // Arbitrarily assuming that not logged in regardless of what result occurred. Thus resetting related data
            // (Note that in reality there COULD be race conditions with these callbacks depending on how the SDK is implemented)
            // (Thus this may burn us in the future)
            // TODO: Reset session, lobby, etc data just in case. (Not yet implemented as really should clean those up BEFORE fully logging out)
            ResetLoginData();
        }

        // Based on SDK sample's FP2PNAT::OnIncomingConnectionRequest
        void OnIncomingConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* data) {
            // Sanity checks
            if (data == nullptr) {
                mLogger.AddErrorNetLog("Input data is unexpectedly null");
                return;
            }
            if (!IsCrossPlatformLoggedIn()) {
                mLogger.AddWarnNetLog(
                    "Somehow receiving connection requests without being cross platform logged in. Perhaps outdated expectation?"
                );
                return;
            }

            // Validate socket name. Better to be safe th an sorry
            std::string socketName = data->SocketId->SocketName;
            if (socketName != "CHAT") { // TODO: Support non-hardcoded specific socket
                mLogger.AddWarnNetLog("Ignoring due to unexpected socket name. Incoming name: " + socketName);
                return;
            }

            // Validate the other player. Otherwise we theoretically could be accepting spam connections from anyone
            CrossPlatformIdWrapper remoteIdWrapped(data->RemoteUserId);
            if (!IsValidPlayerForIncomingConnectionRequest(remoteIdWrapped)) {
                mLogger.AddWarnNetLog("Ignoring invalid player with id: " + remoteIdWrapped.ToStringForLogging());
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);
            EOS_P2P_AcceptConnectionOptions options;
            options.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
            options.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
            options.RemoteUserId = data->RemoteUserId;

            EOS_P2P_SocketId socketId;
            socketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
            // strncpy_s(socketId.SocketName, TEST_SOCKET_NAME.c_str(), TEST_SOCKET_NAME.length() + 1);
            strncpy_s(socketId.SocketName, "CHAT", 5);
            options.SocketId = &socketId;

            EOS_EResult result = EOS_P2P_AcceptConnection(p2pHandle, &options);
            if (result == EOS_EResult::EOS_Success) {
                mLogger.AddInfoNetLog("Successfully accepted connection from: " + remoteIdWrapped.ToStringForLogging());
            }
            else {
                mLogger.AddErrorNetLog("Accepting connection failed with result: " + EOSHelpers::ResultCodeToString(result));
            }
        }
        void OnPeerConnectionEstablished(const EOS_P2P_OnPeerConnectionEstablishedInfo* data) {
            // Sanity checks
            if (data == nullptr) {
                mLogger.AddErrorNetLog("Input data is unexpectedly null");
                return;
            }
            if (!IsCrossPlatformLoggedIn()) {
                mLogger.AddWarnNetLog(
                    "Somehow receiving connection requests without being cross platform logged in. Perhaps outdated expectation?"
                );
                return;
            }

            CrossPlatformIdWrapper remoteId = CrossPlatformIdWrapper(data->RemoteUserId);
            std::string socketName = data->SocketId->SocketName;
            std::string connectionEstablishedType =
                data->ConnectionType == EOS_EConnectionEstablishedType::EOS_CET_NewConnection ? "NewConnection" : "Reconnection";

            std::string networkConnectionType = "<MissingSwitchCase>";
            switch (data->NetworkType) {
                case EOS_ENetworkConnectionType::EOS_NCT_NoConnection:
                    networkConnectionType = "NoConnection";
                    break;
                case EOS_ENetworkConnectionType::EOS_NCT_DirectConnection:
                    networkConnectionType = "DirectConnection";
                    break;
                case EOS_ENetworkConnectionType::EOS_NCT_RelayedConnection:
                    networkConnectionType = "RelayedConnection";
                    break;
            }
            
            mLogger.AddInfoNetLog(
                "Connection established with remote id " + remoteId.ToStringForLogging() + " | socket name: " + socketName
                + " | ConnectionEstablishedType: " + connectionEstablishedType
                + " | NetworkConnectionType: " + networkConnectionType
            );
        }
        void OnPeerConnectionInterrupted(const EOS_P2P_OnPeerConnectionInterruptedInfo* data) {
            // Sanity checks
            if (data == nullptr) {
                mLogger.AddErrorNetLog("Input data is unexpectedly null");
                return;
            }
            if (!IsCrossPlatformLoggedIn()) {
                mLogger.AddWarnNetLog(
                    "Somehow receiving connection requests without being cross platform logged in. Perhaps outdated expectation?"
                );
                return;
            }

            CrossPlatformIdWrapper remoteId = CrossPlatformIdWrapper(data->RemoteUserId);
            std::string socketName = data->SocketId->SocketName;
            mLogger.AddWarnNetLog(
                "Connection interrupted with remote id " + remoteId.ToStringForLogging() + " | socket name: " + socketName
            );
        }
        void OnPeerConnectionClosed(const EOS_P2P_OnRemoteConnectionClosedInfo* data) {
            // Sanity checks
            if (data == nullptr) {
                mLogger.AddErrorNetLog("Input data is unexpectedly null");
                return;
            }
            if (!IsCrossPlatformLoggedIn()) {
                mLogger.AddWarnNetLog(
                    "Somehow receiving connection requests without being cross platform logged in. Perhaps outdated expectation?"
                );
                return;
            }

            CrossPlatformIdWrapper remoteId = CrossPlatformIdWrapper(data->RemoteUserId);
            std::string socketName = data->SocketId->SocketName;
            mLogger.AddInfoNetLog(
                "Connection closed with remote id " + remoteId.ToStringForLogging() + " | socket name: " + socketName
                + " | ConnectionClosedReason: " + std::to_string(static_cast<int>(data->Reason))
            );
        }
        void OnIncomingPacketQueueFull(const EOS_P2P_OnIncomingPacketQueueFullInfo* data) {
            // Sanity checks
            if (data == nullptr) {
                mLogger.AddErrorNetLog("Input data is unexpectedly null");
                return;
            }
            if (!IsCrossPlatformLoggedIn()) {
                mLogger.AddWarnNetLog(
                    "Somehow receiving connection requests without being cross platform logged in. Perhaps outdated expectation?"
                );
                return;
            }
            
            mLogger.LogWarnMessage(
                "Incoming packet queue full! PacketQueueMaxSizeBytes: " + std::to_string(data->PacketQueueMaxSizeBytes)
                + " | PacketQueueCurrentSizeBytes: " + std::to_string(data->PacketQueueCurrentSizeBytes)
                + " | OverflowPacketChannel: " + std::to_string(data->OverflowPacketChannel)
                + " | OverflowPacketSizeBytes: " + std::to_string(data->OverflowPacketSizeBytes)
            );
        }

        void OnShowFriendsResult(const EOS_UI_ShowFriendsCallbackInfo* data) {
            if (data->ResultCode == EOS_EResult::EOS_Success) {
                mLogger.AddInfoNetLog("EWS::OnShowFriendsCallback",  "Success!");
            }
            else {
                mLogger.AddWarnNetLog("Failed with result code: " + EOSHelpers::ResultCodeToString(data->ResultCode));
            }
        }

        // Based on SDK sample's FUsers::OnQueryAccountMappingsCallback
        void OnQueryAccountMappingsResult(const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                return;
            }
            if (!EOS_EResult_IsOperationComplete(data->ResultCode)) {
                mLogger.AddInfoNetLog(
                    "Operation not complete, should be automatically called again in near future. Result code: "
                    + EOSHelpers::ResultCodeToString(data->ResultCode)
                );
                return; // For now just wait until completion
            }

            // Overall fail case
            if (data->ResultCode != EOS_EResult::EOS_Success) {
                mLogger.AddWarnNetLog("Failed with error code: " + EOSHelpers::ResultCodeToString(data->ResultCode));

                // Don't have ability to see which requests failed exactly, so clear all of em
                mPlayersInfoTracking.ClearAllPendingAccountIdRequestsDueToFailure();
                return;
            }

            mLogger.AddInfoNetLog("Called with initial success");
            EOS_HConnect connectHandle = EOS_Platform_GetConnectInterface(mPlatformHandle);
            
            // Ordinary API callback doesn't return provided ids. Thus loop through all manually tracked ids to retrieve
            // the corresponding results
            //      Note that grabbing a copy of the "list" here as we'll modify this in our foreach loop by the
            //      non-const call to the info tracking. Had some fun inconsistent crashes until discovered this.
            const std::set<CrossPlatformIdWrapper> allQueriedAccountMappings =
                mPlayersInfoTracking.GetCurrentlyQueriedAccountMappings();
            for (const CrossPlatformIdWrapper& queriedId : allQueriedAccountMappings) {
                // Setup input API options
                EOS_Connect_GetProductUserIdMappingOptions Options = {};
                Options.ApiVersion = EOS_CONNECT_GETEXTERNALACCOUNTMAPPINGS_API_LATEST;
                Options.AccountIdType = EOS_EExternalAccountType::EOS_EAT_EPIC;
                Options.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
                Options.TargetProductUserId = queriedId.GetAccountId();

                // Try retrieving the account mapping result
                char buffer[EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH];
                int32_t idStringSize = EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH;
                EOS_EResult resultCode = EOS_Connect_GetProductUserIdMapping(connectHandle, &Options, buffer, &idStringSize);
                
                if (resultCode == EOS_EResult::EOS_Success) // id actually received?
                {
                    std::string idString(buffer, idStringSize);
                    EpicAccountIdWrapper resultEpicAccountId = EpicAccountIdWrapper::FromString(idString);
                    mPlayersInfoTracking.SetAccountIdMapping(mLogger, resultEpicAccountId, queriedId);

                    mLogger.AddInfoNetLog("Successfully retrieved a mapping!");
                }
                else {
                    // Theoretically could not have succeeded as still waiting for result from a separate query.
                    // Thus log at warning level so can definitely investigate if/when this happens.
                    mLogger.AddWarnNetLog(
                        "EOS_Connect_GetProductUserIdMapping did not succeed with warning code "
                        + EOSHelpers::ResultCodeToString(data->ResultCode) + " for cross plat id "  + queriedId.ToStringForLogging()
                        + ". Need to investigate if this is an expected situation (such as waiting for results still) or if unexpected"
                    );
                }
            }
        }
        // Based on SDK sample's FUsers::QueryUserInfoCompleteCallbackFn and FUsers::OnQueryDisplayNameFinishedCallback
        void OnQueryUserInfoResult(const EOS_UserInfo_QueryUserInfoCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                return;
            }
            if (!EOS_EResult_IsOperationComplete(data->ResultCode)) {
                mLogger.AddInfoNetLog(
                    "Operation not complete, should be automatically called again in near future. Result code: "
                    + EOSHelpers::ResultCodeToString(data->ResultCode)
                );
                return; // For now just wait until completion
            }

            // General fail case
            if (data->ResultCode != EOS_EResult::EOS_Success) {
                mLogger.AddWarnNetLog("Failed with error code: " + EOSHelpers::ResultCodeToString(data->ResultCode));
                mPlayersInfoTracking.ClearPendingUserInfoRequestDueToFailure(data->TargetUserId);
                return;
            }
            // Sanity check. Not strictly necessary for LocalUserId API param in a way, but only doing these queries
            //      atm when logged in anyways.
            if (!IsBasicLoggedIn()) {
                mLogger.AddWarnNetLog("Not logged in atm!");
                mPlayersInfoTracking.ClearPendingUserInfoRequestDueToFailure(data->TargetUserId);
                return;
            }

            // Prep copying the actual  user info
            EOS_HUserInfo userInfoInterface = EOS_Platform_GetUserInfoInterface(mPlatformHandle);
            EOS_UserInfo_CopyUserInfoOptions options = {};
            options.ApiVersion = EOS_USERINFO_COPYUSERINFO_API_LATEST;
            options.LocalUserId = mLoggedInEpicAccountId.GetAccountId();
            options.TargetUserId = data->TargetUserId;

            // Try to do the actual info retrieval
            EOS_UserInfo* userInfo = nullptr;
            EOS_EResult copyInfoResultCode = EOS_UserInfo_CopyUserInfo(userInfoInterface, &options, &userInfo);
            if (copyInfoResultCode != EOS_EResult::EOS_Success || !userInfo) {
                mLogger.AddWarnNetLog("Copying user info failed with error code: " + EOSHelpers::ResultCodeToString(copyInfoResultCode));
                mPlayersInfoTracking.ClearPendingUserInfoRequestDueToFailure(data->TargetUserId);
                return;
            }
            mLogger.AddInfoNetLog("Successfully retrieved info!");

            // Process the info
            mPlayersInfoTracking.SetQueriedUserInfo(mLogger, data->TargetUserId, *userInfo);
            EOS_UserInfo_Release(userInfo);

            // If all user info queries completed, then let "user" side know so they can update name display or such
            //      as necessary.
            //   Side note: Not doing a callback every query as an early choice *assuming* no need to update one by one.
            if (!mPlayersInfoTracking.HasAnyPendingRequests(mLogger)) {
                if (mEosWrapperManager) {
                    mEosWrapperManager->OnAllPlayerInfoQueriesCompleted(mPlayersInfoTracking);
                }
            }
        }

        // Based on SDK sample's FLobbies::OnCreateLobbyFinished
        void OnCreateLobbyFinished(const EOS_Lobby_CreateLobbyCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                mEosWrapperManager->OnLobbyCreationResult(false, {}, mPlayersInfoTracking);
                return;
            }
            if (!EOS_EResult_IsOperationComplete(data->ResultCode)) {
                mLogger.AddInfoNetLog(
                    "Operation not complete, should be automatically called again in near future. Result code: "
                    + EOSHelpers::ResultCodeToString(data->ResultCode)
                );
                return; // For now just wait until completion
            }

            bool didSetupSucceed = data->ResultCode == EOS_EResult::EOS_Success;

            // Give bit of precise feedback to help with general debugging
            if (!didSetupSucceed) { // Fail case
                mLogger.AddWarnNetLog("Creation failed with error code: " + EOSHelpers::ResultCodeToString(data->ResultCode));
            }
            else { // Success case
                mLogger.AddInfoNetLog("Lobby successfully created!");
            }

            bool wasLobbyJoin = false;
            HandlePostLobbyJoinOrCreation(wasLobbyJoin, didSetupSucceed, NetLobbySlot::MainMatchLobby, data->LobbyId);
        }
        // Based on SDK sample's FLobbies::OnJoinLobbyFinished
        void OnJoinLobbyFinished(const EOS_Lobby_JoinLobbyCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                mEosWrapperManager->OnLobbyJoinResult(false, {}, mPlayersInfoTracking);
                return;
            }

            if (!EOS_EResult_IsOperationComplete(data->ResultCode)) {
                mLogger.AddInfoNetLog(
                    "Operation not complete, should be automatically called again in near future. Result code: "
                    + EOSHelpers::ResultCodeToString(data->ResultCode)
                );
                return; // For now just wait until completion
            }

            bool didJoinSucceed = data->ResultCode == EOS_EResult::EOS_Success;

            // Give bit of precise feedback to help with general debugging
            if (!didJoinSucceed) { // Fail case
                mLogger.AddWarnNetLog("Join failed with error code: " + EOSHelpers::ResultCodeToString(data->ResultCode));
            }
            else { // Success case
                mLogger.AddInfoNetLog("Lobby successfully joined!");
            }

            bool wasLobbyJoin = true;
            HandlePostLobbyJoinOrCreation(wasLobbyJoin, didJoinSucceed, NetLobbySlot::MainMatchLobby, data->LobbyId);
        }
        // Based on SDK sample's FLobbies::OnLeaveLobbyFinished
        void OnLeaveLobbyFinished(const EOS_Lobby_LeaveLobbyCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                mEosWrapperManager->OnLobbyLeftOrDestroyed(false);
                return;
            }

            if (!EOS_EResult_IsOperationComplete(data->ResultCode)) {
                mLogger.AddWarnNetLog(
                    "Operation not complete, should be automatically called again in near future. Result code: "
                    + EOSHelpers::ResultCodeToString(data->ResultCode)
                );
                return; // For now just wait until completion
            }

            bool didLeaveSucceed = data->ResultCode == EOS_EResult::EOS_Success;

            // Give bit of precise feedback to help with general debugging
            if (!didLeaveSucceed) {
                mLogger.AddWarnNetLog("Leave failed with error code: " + EOSHelpers::ResultCodeToString(data->ResultCode));
            }
            else {
                mLogger.AddInfoNetLog("Successfully left lobby!");
            }

            NetLobbySlot netLobbySlot = FakePointerToLobbySlot(data->ClientData);
            HandlePostLobbyLeave(didLeaveSucceed, netLobbySlot);
        }

        // Based on SDK sample's FLobbies::OnLobbyInviteReceived
        void OnLobbyInviteReceived(const EOS_Lobby_LobbyInviteReceivedCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                return;
            }

            mLogger.AddInfoNetLog("TODO");
        }
        void OnLobbyInviteAccepted(const EOS_Lobby_LobbyInviteAcceptedCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                return;
            }

            // Retrieve the lobby's handle as that's required for the Join API
            EOS_Lobby_CopyLobbyDetailsHandleByInviteIdOptions Options = {};
            Options.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLEBYINVITEID_API_LATEST;
            Options.InviteId = data->InviteId;
            
            EOS_HLobbyDetails lobbyDetailsHandle = nullptr;
            EOS_EResult resultCode = EOS_Lobby_CopyLobbyDetailsHandleByInviteId(mLobbyHandle, &Options, &lobbyDetailsHandle);
            if (resultCode != EOS_EResult::EOS_Success)
            {
                mLogger.AddWarnNetLog(
                    "Lobby details handle retrieval failed with error code: " + EOSHelpers::ResultCodeToString(resultCode)
                );
                return;
            }
            if (lobbyDetailsHandle == nullptr)
            {
                mLogger.AddWarnNetLog("Lobby details handle retrieval failed due to nullptr result");
                return;
            }

            // Start actual join
            bool didJoinAttemptStart = TryJoinLobby(NetLobbySlot::MainMatchLobby, MakeLobbyDetailsKeeper(lobbyDetailsHandle));
            // Give bit of feedback (as not much UI support atm)
            if (didJoinAttemptStart) {
                mLogger.AddInfoNetLog("Successfully started join attempt");
            }
            else {
                mLogger.AddWarnNetLog("Failed to start join attempt. Should have prior logs");
            }
        }
        void OnJoinLobbyAccepted(const EOS_Lobby_JoinLobbyAcceptedCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                return;
            }

            // Retrieve the lobby's handle as that's required for the Join API
            EOS_Lobby_CopyLobbyDetailsHandleByUiEventIdOptions copyOptions = {};
            copyOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLEBYUIEVENTID_API_LATEST;
            copyOptions.UiEventId = data->UiEventId;
            
            EOS_HLobbyDetails lobbyDetailsHandle;
            EOS_EResult resultCode = EOS_Lobby_CopyLobbyDetailsHandleByUiEventId(mLobbyHandle, &copyOptions, &lobbyDetailsHandle);
            if (resultCode != EOS_EResult::EOS_Success)
            {
                mLogger.AddWarnNetLog(
                    "Lobby details handle retrieval failed with error code: " + EOSHelpers::ResultCodeToString(resultCode)
                );
                return;
            }
            if (lobbyDetailsHandle == nullptr)
            {
                mLogger.AddWarnNetLog("Lobby details handle retrieval failed due to nullptr result");
                return;
            }

            // Start actual join
            bool didJoinAttemptStart = TryJoinLobby(NetLobbySlot::MainMatchLobby, MakeLobbyDetailsKeeper(lobbyDetailsHandle));
            // Give bit of feedback (as not much UI support atm)
            if (didJoinAttemptStart) {
                mLogger.AddInfoNetLog("Successfully started join attempt");
            }
            else {
                mLogger.AddWarnNetLog("Failed to start join attempt. Should have prior logs");
            }
        }

        // Based on SDK sample's FLobbies::OnLobbyUpdateReceived
        void OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                return;
            }

            OnLobbyUpdated(NetLobbySlot::MainMatchLobby, data->LobbyId);
        }
        void OnMemberUpdateReceived(const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                return;
            }

            // TODO: Remove log
            mLogger.AddInfoNetLog("Called");

            OnLobbyUpdated(NetLobbySlot::MainMatchLobby, data->LobbyId);
        }
        void OnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* data) {
            if (!data) {
                mLogger.AddWarnNetLog("Input data is somehow null!");
                return;
            }

            // TODO: Remove log
            mLogger.AddInfoNetLog("Called");

            // TODO: If target user id == self AND current status == closed/kicked/disconnected, then handle as if kicked or lobby closed or such

            // Otherwise still in lobby, sp do overall lobby update
            OnLobbyUpdated(NetLobbySlot::MainMatchLobby, data->LobbyId);
        }
        #pragma endregion

      private:
        /// <returns>True if EOS Platform successfully initialized</returns>
        bool TryCreatePlatformInstance() {
            EOS_Platform_Options platformOptions = {};
            platformOptions.ApiVersion = EOS_PLATFORM_OPTIONS_API_LATEST;

            platformOptions.ProductId = NetworkSecrets::ProductId;
            platformOptions.SandboxId = NetworkSecrets::SandboxId;
            platformOptions.DeploymentId = NetworkSecrets::DeploymentId;
            platformOptions.ClientCredentials.ClientId = NetworkSecrets::ClientCredentialsId;
            platformOptions.ClientCredentials.ClientSecret = NetworkSecrets::ClientCredentialsSecret;
            platformOptions.EncryptionKey = NetworkSecrets::EncryptionKey;

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

            mPlatformHandle = EOS_Platform_Create(&platformOptions);
            if (mPlatformHandle == nullptr) {
                mLogger.AddErrorNetLog("EOS Platform failed to init");
                return false;
            }

            mLogger.AddInfoNetLog("EOS Platform successfully initialized!");
            return true;
        }


        
        #pragma region Logic-related Methods (Private)
        void SetCredentialsForAccountPortalLogin(EOS_Auth_Credentials& credentials) {
            credentials.Type = EOS_ELoginCredentialType::EOS_LCT_AccountPortal;
        }
        void SetCredentialsForDevAuthLogin(EOS_Auth_Credentials& credentials,
                                           const std::string& accountName,
                                           const std::string& ipAndPort) {
            credentials.Id = ipAndPort.c_str();
            credentials.Token = accountName.c_str();
            credentials.Type = EOS_ELoginCredentialType::EOS_LCT_Developer;
        }

        /**
        * Begins a login attempt with the provided credentials.
        * @param credentials - "Credentials" to use in login attempt.
        *                       Note that this needs to be pass by reference as it contains pointers (c-strings).
        * @returns true if login attempt begins successfully (which it should in nearly all cases), false otherwise
        **/
        bool BeginLoginAttempt(const EOS_Auth_Credentials& credentials) {
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Not initialized");
                return false;
            }
            if (IsLoggedInAtAll()) {
                mLogger.AddWarnNetLog("Already logged in");
                return false;
            }

            // TODO: Just realized we're getting these before we even logged in. Thus, might as well throw into initialization code!
            mAuthHandle = EOS_Platform_GetAuthInterface(mPlatformHandle);
            mConnectHandle = EOS_Platform_GetConnectInterface(mPlatformHandle);
            mUIHandle = EOS_Platform_GetUIInterface(mPlatformHandle);
            mLobbyHandle = EOS_Platform_GetLobbyInterface(mPlatformHandle);

            // TODO: Setup AddNotifyLoginStatusChanged() (login status callback) if/as necessary
            // See SDK sample FAuthentication::Login's call here towards the beginning
            // Also read SDK's "Status Change Notification" section:
            // https://dev.epicgames.com/docs/services/en-US/EpicAccountServices/AuthInterface/index.html#statuschangenotification

            EOS_Auth_LoginOptions loginOptions = {};
            // Example memset this to 0 for some reason and IDE suggested initializing like this instead
            loginOptions.ApiVersion = EOS_AUTH_LOGIN_API_LATEST;
            // Scope flags are arbitrarily chosen based on descriptions and example login code
            // In future, we should further examine the various options and use what's most appropriate
            // (Note that EOS SDK actually goes into decent detail on this, eg user will see all requested perms. Good read!)
            loginOptions.ScopeFlags = EOS_EAuthScopeFlags::EOS_AS_BasicProfile | EOS_EAuthScopeFlags::EOS_AS_FriendsList
                | EOS_EAuthScopeFlags::EOS_AS_Presence;

            loginOptions.Credentials = &credentials;
            EOS_Auth_Login(mAuthHandle, &loginOptions, this, [](const EOS_Auth_LoginCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnLoginComplete(data);
            });
            
            // TODO: Setup AddConnectAuthExpirationNotification() (connect auth expiration callback) if/as necessary
            // Waiting until this is really necessary to truly understand it before implementing

            // NOTE: If EOS_Auth_Login callback is not called, then make sure credentials are correctly set AND EOS_Platform_Tick is called
            mLogger.AddInfoNetLog("Successfully sent login request");
            return true;
        }
        void OnSuccessfulEpicLogin(const EOS_EpicAccountId resultUserId) {
            mLoggedInEpicAccountId = EpicAccountIdWrapper(resultUserId);

            std::string userIdAsString;
            if (!mLoggedInEpicAccountId.TryToString(userIdAsString)) {
               mLogger.AddErrorNetLog("Could not convert user id to string");
            }
            mLogger.AddInfoNetLog("Login Complete - User ID: " + userIdAsString);
            
            StartCrossPlatformLogin();
        }
        void OnSuccessfulCrossPlatformLogin(const EOS_ProductUserId resultCrossPlatformId) {
            mLoggedInCrossPlatformId = CrossPlatformIdWrapper(resultCrossPlatformId);

            std::string userIdAsString;
            if (!mLoggedInCrossPlatformId.TryToString(userIdAsString)) {
                mLogger.AddErrorNetLog("Could not convert cross platform id to string");
            }
            mLogger.AddInfoNetLog("Login Complete - Cross Platform User ID: " + userIdAsString);

            // Subscribe to various relevant updates that may happen during netcode usage
            SubscribeToConnectionRequests(); // Must be done after logging in, otherwise get EOS SDK log warnings/errors
            SubscribeToLobbyInvites();
            SubscribeToLobbyUpdates();

            if (mEosWrapperManager) {
                mEosWrapperManager->OnLoginSuccess(mLoggedInCrossPlatformId);
            }
        }
        // Based on SDK sample's FAuthentication::ConnectLogin
        void StartCrossPlatformLogin() {
            // Sanity checks
            if (!IsBasicLoggedIn()) {
                mLogger.AddErrorNetLog("Is not 'basic' logged in yet");
                return;
            }
            if (mAuthHandle == nullptr) {
                mLogger.AddErrorNetLog("AuthHandle null");
                return;
            }
            if (mConnectHandle == nullptr) {
                mLogger.AddErrorNetLog("ConnectHandle null");
                return;
            }
            
            EOS_Auth_Token* userAuthToken = nullptr;
            
            EOS_Auth_CopyUserAuthTokenOptions copyTokenOptions = { 0 };
            copyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;
            EOS_EResult result = EOS_Auth_CopyUserAuthToken(
                mAuthHandle, &copyTokenOptions, mLoggedInEpicAccountId.GetAccountId(), &userAuthToken
            );
            
            if (result == EOS_EResult::EOS_Success) {
                EOS_Connect_Credentials credentials;
                credentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
                credentials.Token = userAuthToken->AccessToken;
                credentials.Type = EOS_EExternalCredentialType::EOS_ECT_EPIC;

                EOS_Connect_LoginOptions options = { 0 };
                options.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
                options.Credentials = &credentials;
                options.UserLoginInfo = nullptr;

                EOS_Connect_Login(mConnectHandle, &options, nullptr, [](const EOS_Connect_LoginCallbackInfo* data) {
                    Singleton<EOSWrapperSingleton>::get().OnCrossPlatformLoginComplete(data);
                });
                EOS_Auth_Token_Release(userAuthToken);
            }
            else {
                mLogger.AddErrorNetLog(
                    "EOS_Auth_CopyUserAuthToken failed with result: " + EOSHelpers::ResultCodeToString(result)
                );
            }
        }

        void ResetLoginData() {
            // Don't explicitly reset lobby and sessions tracking here as that should be separately handled.
            //      After all, if user has a "presence" lobby and logs out without clearing that lobby,
            //          when they log back in and try to create another "presence" lobby, it'll still fail.
            //      Thus, lobbies (abd sessions) *must* be properly cleaned up for proper behavior.

            // Unsubscribe from various updates that we shouldn't handle anymore.
            //      Not sure if strictly necessary even in long-lived editor context, but SDK samples do this and best to be safe.
            UnsubscribeFromConnectionRequests();
            UnsubscribeFromLobbyInvites();
            UnsubscribeToLobbyUpdates();
            
            // Clear handles dependant on logged in state
            mAuthHandle = nullptr;
            mConnectHandle = nullptr;
            mUIHandle = nullptr;
            mLobbyHandle = nullptr;

            // Clear relevant ids
            mLoggedInCrossPlatformId = {};
            mLoggedInEpicAccountId = {};
        }
        #pragma endregion

        #pragma region P2P-related Methods (Private)
        bool IsValidPlayerForIncomingConnectionRequest(const CrossPlatformIdWrapper& otherPlayerId) const {
            // If not in a match lobby currently, then reject all connections as currently only accepting p2p connections
            // from game match lobby members
            if (!mMainMatchLobby.IsActive()) {
                mLogger.AddWarnNetLog("Invalid request due to not being in a match lobby atm");
                return false;
            }

            // Only accept connections from a game match lobby member
            //      Don't want to accept potential spam connections from any ol' person on the internet and this is an
            //      easy way to define what are "expected" or "valid" connections
            const std::vector<CrossPlatformIdWrapper>& lobbyMembers = mMainMatchLobby.GetLobbyProperties().GetMembersList();
            for (const CrossPlatformIdWrapper& memberId : lobbyMembers) {
                if (otherPlayerId == memberId) {
                    return true;
                }
            }

            mLogger.AddWarnNetLog("Invalid request due to player id not being part of current lobby members list");
            return false;
        }
        
        // Based on SDK sample's FP2PNAT::HandleReceivedMessages
        void HandleReceivedMessages() {
            if (!IsFullyLoggedIn()) { // Nothing to do atm if not even logged in (as can't make p2p connections)
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);

            EOS_P2P_ReceivePacketOptions options;
            options.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
            options.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
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
            
            if (result == EOS_EResult::EOS_NotFound) {
                // No more packets, potential expected case. Nothing to do
            }
            else if (result != EOS_EResult::EOS_Success) {
                mLogger.AddErrorNetLog("Receiving packet failed with result: " + EOSHelpers::ResultCodeToString(result));
            }
            else { // Success case!
                // Chop off unused data at end of array so we don't need to pass around bytes written as well.
                //      FUTURE: Double check performance on this. Apparently linear, following quote from: https://en.cppreference.com/w/cpp/container/vector/resize
                //      "Linear in the difference between the current size and count." (Plus note about reallocation cost)
                messageData.resize(bytesWritten);
                
                if (mEosWrapperManager) {
                    mEosWrapperManager->OnMessageReceived(CrossPlatformIdWrapper(peerId), messageData);
                }
            }
        }
        #pragma endregion

        #pragma region Lobby-related Methods (Private)
        bool BeginCreateLobby(NetLobbySlot lobbySlot) {
            // General sanity checks
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Not initialized");
                return false;
            }
            if (!IsFullyLoggedIn()) {
                mLogger.AddWarnNetLog("Not yet logged in");
                return false;
            }
            if (mLobbyHandle == nullptr) {
                mLogger.AddWarnNetLog("mLobbyHandle is nullptr!");
                return false;
            }

            EOSLobbyTracking& lobbyTrackingInfo = GetLobbyInfoForSlot(lobbySlot);
            // Specific lobby type sanity check
            if (!lobbyTrackingInfo.IsCompletelyInactive()) {
                mLogger.AddWarnNetLog(
                    "Lobby not entirely inactive. Lobby slot: " + std::to_string(static_cast<int>(lobbySlot)) +
                    ", Current status: " + std::to_string(lobbyTrackingInfo.GetCurrentStatusAsNumber())
                );
                return false;
            }


            // Setup lobby options. Based on SDK sample's FLobbies::CreateLobby
            // FUTURE: Refactor relevant options for more flexible setup at EOS Wrapper level
            EOS_Lobby_CreateLobbyOptions createOptions = {};
            createOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
            createOptions.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
            createOptions.MaxLobbyMembers = 4;
            createOptions.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE;
            createOptions.bPresenceEnabled = kIsLobbyPresenceEnabled;
            createOptions.bAllowInvites = true;
            createOptions.BucketId = kDefaultGameLobbyBucketId;
            // Don't see any use case for turning this on atm (eg, instead have host via manager explicitly shut down lobby on leave)
            createOptions.bDisableHostMigration = EOS_FALSE;
            // Not using RTC (real-time chat) features atm
            createOptions.bEnableRTCRoom = EOS_FALSE;
            createOptions.LocalRTCOptions = nullptr;

            // Track lobby creation for general validation purposes
            lobbyTrackingInfo.SetBeingCreatedState();
            // Give feedback to "user" layer in case they need to know that this action is actually going to begin.
            //      Originally implemented for join side as player can do that through social overlay, but best be
            //      consistent here for "user" side.
            //
            //      Side note: Doing before EOS SDK call to extra guarantee that the finish callback won't occur before
            //          this begin callback
            if (mEosWrapperManager) {
                mEosWrapperManager->OnLobbyJoinOrCreateBegin();
            }

            // Actually send the lobby creation request
            auto simpleCallback = [](const EOS_Lobby_CreateLobbyCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnCreateLobbyFinished(data);
            };
            EOS_Lobby_CreateLobby(mLobbyHandle, &createOptions, nullptr, simpleCallback);

            return true;
        }
        bool TryJoinLobby(NetLobbySlot lobbySlot, EOSLobbyDetailsKeeper targetLobbyHandle) {
            // General sanity checks
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Not initialized");
                return false;
            }
            if (!IsFullyLoggedIn()) {
                mLogger.AddWarnNetLog("Not yet logged in");
                return false;
            }
            if (mLobbyHandle == nullptr) {
                mLogger.AddWarnNetLog("mLobbyHandle is nullptr!");
                return false;
            }

            EOSLobbyTracking& lobbyTrackingInfo = GetLobbyInfoForSlot(lobbySlot);

            // If lobby already exists in some fashion, then leave that one first before joining this one
            if (!lobbyTrackingInfo.IsCompletelyInactive()) {
                // Lobby tracking: Remember new pending lobby to join
                lobbyTrackingInfo.SetActiveDelayedJoinRequest(targetLobbyHandle);

                // If lobby is active and in use, then start the leave flow
                if (lobbyTrackingInfo.IsActive()) {
                    BeginLeaveLobby(lobbySlot);
                }
                
                // Otherwise, nothing to do. Once lobby reaches active state, we'll begin the new join.
                //      See HandlePostLobbyInActiveState() method for more.
                return true;
            }

            // Otherwise, nothing should stop us from trying to immediately join the desired lobby...
            EOS_Lobby_JoinLobbyOptions JoinOptions = {};
            JoinOptions.ApiVersion = EOS_LOBBY_JOINLOBBY_API_LATEST;
            JoinOptions.LobbyDetailsHandle = targetLobbyHandle.get();
            JoinOptions.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
            JoinOptions.bPresenceEnabled = kIsLobbyPresenceEnabled;

            // Adjust the lobby tracking so it matches expected flow
            lobbyTrackingInfo.SetJoinInProgressState();
            // Give feedback to "user" layer in case they need to know that this action is actually going to begin.
            //      Ex: If player tries to join a lobby via Friends overlay, then frontend may not know about this yet.
            //
            //      Side note: Doing before EOS SDK call to extra guarantee that the finish callback won't occur before
            //          this begin callback.
            if (mEosWrapperManager) {
                mEosWrapperManager->OnLobbyJoinOrCreateBegin();
            }
            
            // Actually send the join request
            auto simpleCallback = [](const EOS_Lobby_JoinLobbyCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnJoinLobbyFinished(data);
            };
            EOS_Lobby_JoinLobby(mLobbyHandle, &JoinOptions, nullptr, simpleCallback);

            return true;
        }

        // Based on SDK samples' FLobbies::LeaveLobby
        bool BeginLeaveLobby(NetLobbySlot lobbySlot) {
            EOSLobbyTracking& lobbyTrackingInfo = GetLobbyInfoForSlot(lobbySlot);
            
            // General sanity checks
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Not initialized");
                return false;
            }
            if (!IsFullyLoggedIn()) {
                mLogger.AddWarnNetLog("Not yet logged in");
                return false;
            }
            if (mLobbyHandle == nullptr) {
                mLogger.AddWarnNetLog("mLobbyHandle is nullptr!");
                return false;
            }
            // Specific lobby sanity checks
            if (!lobbyTrackingInfo.IsActive()) {
                mLogger.AddWarnNetLog("Given lobby is not currently active");
                return false;
            }

            // Update state before actual leave lobby in case callback is immediately called for some reason
            mMainMatchLobby.SetLeaveInProgressState();

            // TODO: If <= 1 members in lobby (and owner just in case), then destroy lobby instead. Also figure out why samples do this.

            EOS_Lobby_LeaveLobbyOptions leaveOptions = {};
            leaveOptions.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
            leaveOptions.LobbyId = lobbyTrackingInfo.GetLobbyProperties().GetId().c_str();
            leaveOptions.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();

            // Let "user" side know about this event as we may have initiated the lobby leave from EOS netcode side.
            //      Ex: If player tries to join a lobby via Friends overlay while in a lobby already
            //
            //      Side note: Doing before EOS SDK call to extra guarantee that the finish callback won't occur before
            //          this begin callback
            if (mEosWrapperManager) {
                mEosWrapperManager->OnLobbyLeaveBegin();
            }
            
            // Do actual leave call
            auto simpleCallback = [](const EOS_Lobby_LeaveLobbyCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnLeaveLobbyFinished(data);
            };
            EOS_Lobby_LeaveLobby(mLobbyHandle, &leaveOptions, LobbySlotToPointerType(lobbySlot), simpleCallback);
            
            return true;
        }
        bool DestroyLobby(EOSLobbyTracking& lobbyTracking) {
            // TODO:
            return false;
        }
        
        void OnLobbyUpdated(NetLobbySlot lobbySlot, EOS_LobbyId lobbyId) {
            EOSLobbyTracking& lobbyTracking = GetLobbyInfoForSlot(lobbySlot);

            bool didUpdateSucceed =
                lobbyTracking.TryUpdateActiveLobby(mLoggedInCrossPlatformId, mLobbyHandle, lobbyId, mPlayersInfoTracking);
            
            if (!didUpdateSucceed) { // Bit of feedback to help with debugging
                mLogger.AddWarnNetLog("Failed to update lobby, may have downstream issues");
            }
            else {
                mLogger.AddInfoNetLog("Update succeeded on EWS side");
            }

            if (mEosWrapperManager) {
                mEosWrapperManager->OnLobbyUpdated(didUpdateSucceed, lobbyTracking.GetLobbyProperties(), mPlayersInfoTracking);
            }
        }
        // Handles lobby join nor creation
        void HandlePostLobbyJoinOrCreation(bool wasLobbyJoin,
                                           bool didSucceedThusFar,
                                           NetLobbySlot lobbySlot,
                                           EOS_LobbyId newLobbyId) {
            EOSLobbyTracking& lobbyTracking = GetLobbyInfoForSlot(lobbySlot);
            
            // Try to setup lobby info if actually now part of a lobby
            if (didSucceedThusFar) {
                // Try to setup lobby info tracking
                didSucceedThusFar = mMainMatchLobby.TryInitActiveLobby(
                    mLoggedInCrossPlatformId, mLobbyHandle, newLobbyId, mPlayersInfoTracking
                );
            }
            
            // If didn't succeed, then make sure to reset tracking so no problems with trying to create future lobbies
            if (!didSucceedThusFar) {
                mMainMatchLobby.ResetLobbyInfo(true);
            }
            else { // Success case!
                HandleLobbyNowInactiveOrActive(lobbySlot);
            }

            // Finally let rest of game handle this event as appropriate
            if (mEosWrapperManager) {
                if (wasLobbyJoin) {
                    mEosWrapperManager->OnLobbyJoinResult(
                        didSucceedThusFar, lobbyTracking.GetLobbyProperties(), mPlayersInfoTracking
                    );
                }
                else {
                    mEosWrapperManager->OnLobbyCreationResult(
                        didSucceedThusFar, lobbyTracking.GetLobbyProperties(), mPlayersInfoTracking
                    );
                }
            }
        }
        void HandlePostLobbyLeave(bool didLeaveSucceed, NetLobbySlot lobbySlot) {
            EOSLobbyTracking& lobbyTracking = GetLobbyInfoForSlot(lobbySlot);
            
            if (!didLeaveSucceed) {
                // Note: No idea what should do atm if leaving lobby fails. Need to see an actual case in wild (or docs) methinks.
                //       Likely just have a backup "force leave" option for frontend at least.
                mLogger.AddWarnNetLog(
                    "Lobby leave somehow failed, not doing anything atm! Review situation and decide what to implement"
                );
            }
            else {
                // Reset lobby so we can reuse it going forward (ie, create a new lobby without validation issues)
                lobbyTracking.ResetLobbyInfo(false);

                HandleLobbyNowInactiveOrActive(lobbySlot);
            }

            // Finally let rest of game handle this event as appropriate
            if (mEosWrapperManager) {
                mEosWrapperManager->OnLobbyLeftOrDestroyed(didLeaveSucceed);
            }
        }

        void HandleLobbyNowInactiveOrActive(NetLobbySlot lobbySlot) {
            EOSLobbyTracking& lobbyTracking = GetLobbyInfoForSlot(lobbySlot);
            
            // Sanity check
            if (!lobbyTracking.IsActive() && !lobbyTracking.IsCompletelyInactive()) {
                mLogger.AddWarnNetLog(
                    "Lobby tracking not in active or inactive state! Current status: " +
                    std::to_string(lobbyTracking.GetCurrentStatusAsNumber())
                );
                return;
            }

            // Rest of function handles delayed join request. Thus if there's none of that, then nothing to do atm
            if (!lobbyTracking.HasActiveDelayedJoinRequest()) {
                return;
            }

            // Restart the join request, which will handle the current state as appropriate (such as leaving an active lobby)
            TryJoinLobby(lobbySlot, lobbyTracking.GetAndClearDelayedJoinRequest());

            // TODO: Does "user"/frontend side need any callbacks? (Eg, if just joined another lobby but trying to change lobbies)
            //      Could definitely see a bug where join one lobby, match starts, but join takes to different lobby.
        }

        EOSLobbyTracking& GetLobbyInfoForSlot(NetLobbySlot lobbySlot) {
            // Only one slot atm
            return mMainMatchLobby;
        }

        void* LobbySlotToPointerType(NetLobbySlot lobbySlot) {
            // Don't use the address but rather the value itself as a pointer.
            // This allows us to use the EOS SDK's void* parameter as simple data rather than as an actual pointer
            
            uint8_t baseNumericValue = static_cast<uint8_t>(lobbySlot); // Best to be extra explicit and careful
            size_t pointerMatchingSizedValue = static_cast<size_t>(baseNumericValue); // Match step in FakePointerToLobbySlot() for clarity
            return reinterpret_cast<void*>(pointerMatchingSizedValue);
        }
        NetLobbySlot FakePointerToLobbySlot(void* fakePointer) {
            // See comments in LobbySlotToPointerType. Just reversing the process
            size_t pointerMatchingSizedValue = reinterpret_cast<size_t>(fakePointer); // Necessary step to avoid truncation error
            uint8_t baseNumericValue = static_cast<uint8_t>(pointerMatchingSizedValue);
            return static_cast<NetLobbySlot>(baseNumericValue);
        }
        #pragma endregion
        
        #pragma region EOS Subscriptions
        bool IsSubscribedToConnectionRequests() {
            return mP2PConnectionRequestNotificationId != EOS_INVALID_NOTIFICATIONID
                && mP2PConnectionEstablishedNotificationId != EOS_INVALID_NOTIFICATIONID
                && mP2PConnectionInterruptedNotificationId != EOS_INVALID_NOTIFICATIONID
                && mP2PConnectionClosedNotificationId != EOS_INVALID_NOTIFICATIONID
                && mP2PIncomingPacketQueueFullNotificationId != EOS_INVALID_NOTIFICATIONID;
        }
        void SubscribeToConnectionRequests() {
            // Sanity checks
            if (!IsInitialized()) {
                mLogger.AddErrorNetLog("Called but not yet initialized!");
                return;
            }
            if (IsSubscribedToConnectionRequests()) {
                mLogger.AddErrorNetLog("Already subscribed to connection requests");
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);

            EOS_P2P_SocketId socketId;
            socketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
            // strncpy_s(socketId.SocketName, TEST_SOCKET_NAME.c_str(), TEST_SOCKET_NAME.size() + 1);
            strncpy_s(socketId.SocketName, "CHAT", 5); // TODO: Why didn't above custom name setting work?

            EOS_P2P_AddNotifyPeerConnectionRequestOptions connectionReqOptions = {};
            connectionReqOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
            connectionReqOptions.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
            connectionReqOptions.SocketId = &socketId;
            mP2PConnectionRequestNotificationId =
                EOS_P2P_AddNotifyPeerConnectionRequest(p2pHandle, &connectionReqOptions, nullptr,
                    [](const EOS_P2P_OnIncomingConnectionRequestInfo* data) {
                        Singleton<EOSWrapperSingleton>::get().OnIncomingConnectionRequest(data);
                    });

            EOS_P2P_AddNotifyPeerConnectionEstablishedOptions connectionEstablishedOptions;
            connectionEstablishedOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONESTABLISHED_API_LATEST;
            connectionEstablishedOptions.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
            connectionEstablishedOptions.SocketId = &socketId;
            mP2PConnectionEstablishedNotificationId =
                EOS_P2P_AddNotifyPeerConnectionEstablished(p2pHandle, &connectionEstablishedOptions, nullptr,
                    [](const EOS_P2P_OnPeerConnectionEstablishedInfo* data) {
                        Singleton<EOSWrapperSingleton>::get().OnPeerConnectionEstablished(data);
                    });

            EOS_P2P_AddNotifyPeerConnectionInterruptedOptions connectionInterruptedOptions;
            connectionInterruptedOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONINTERRUPTED_API_LATEST;
            connectionInterruptedOptions.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
            connectionInterruptedOptions.SocketId = &socketId;
            mP2PConnectionInterruptedNotificationId =
                EOS_P2P_AddNotifyPeerConnectionInterrupted(p2pHandle, &connectionInterruptedOptions, nullptr,
                    [](const EOS_P2P_OnPeerConnectionInterruptedInfo* data) {
                        Singleton<EOSWrapperSingleton>::get().OnPeerConnectionInterrupted(data);
                    });

            EOS_P2P_AddNotifyPeerConnectionClosedOptions connectionClosedOptions;
            connectionClosedOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONINTERRUPTED_API_LATEST;
            connectionClosedOptions.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
            connectionClosedOptions.SocketId = &socketId;
            mP2PConnectionClosedNotificationId =
                EOS_P2P_AddNotifyPeerConnectionClosed(p2pHandle, &connectionClosedOptions, nullptr,
                    [](const EOS_P2P_OnRemoteConnectionClosedInfo* data) {
                        Singleton<EOSWrapperSingleton>::get().OnPeerConnectionClosed(data);
                    });

            EOS_P2P_AddNotifyIncomingPacketQueueFullOptions incomingPacketQueueFullOptions;
            incomingPacketQueueFullOptions.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONINTERRUPTED_API_LATEST;
            mP2PIncomingPacketQueueFullNotificationId =
                EOS_P2P_AddNotifyIncomingPacketQueueFull(p2pHandle, &incomingPacketQueueFullOptions, nullptr,
                    [](const EOS_P2P_OnIncomingPacketQueueFullInfo* data) {
                        Singleton<EOSWrapperSingleton>::get().OnIncomingPacketQueueFull(data);
                    });
            
            // Success vs failure logging
            if (IsSubscribedToConnectionRequests()) {
                mLogger.AddInfoNetLog("Successfully subscribed");
            }
            else {
                mLogger.AddErrorNetLog("Failed to subscribe, bad notification id returned");
            }
        }
        void UnsubscribeFromConnectionRequests() {
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Called while not currently initialized");
                return;
            }
            if (!IsSubscribedToConnectionRequests()) {
                mLogger.AddWarnNetLog("Not subscribed already");
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);
            EOS_P2P_RemoveNotifyPeerConnectionRequest(p2pHandle, mP2PConnectionRequestNotificationId);
            EOS_P2P_RemoveNotifyPeerConnectionEstablished(p2pHandle, mP2PConnectionEstablishedNotificationId);
            EOS_P2P_RemoveNotifyPeerConnectionInterrupted(p2pHandle, mP2PConnectionInterruptedNotificationId);
            EOS_P2P_RemoveNotifyPeerConnectionClosed(p2pHandle, mP2PConnectionClosedNotificationId);
            EOS_P2P_RemoveNotifyIncomingPacketQueueFull(p2pHandle, mP2PIncomingPacketQueueFullNotificationId);
            
            mP2PConnectionRequestNotificationId = EOS_INVALID_NOTIFICATIONID;
            mP2PConnectionEstablishedNotificationId = EOS_INVALID_NOTIFICATIONID;
            mP2PConnectionInterruptedNotificationId = EOS_INVALID_NOTIFICATIONID;
            mP2PConnectionClosedNotificationId = EOS_INVALID_NOTIFICATIONID;
            mP2PIncomingPacketQueueFullNotificationId = EOS_INVALID_NOTIFICATIONID;
        }

        bool IsSubscribedToLobbyInvites() {
            return mLobbyInviteNotification != EOS_INVALID_NOTIFICATIONID
                && mLobbyInviteAcceptedNotification != EOS_INVALID_NOTIFICATIONID
                && mJoinLobbyAcceptedNotification != EOS_INVALID_NOTIFICATIONID;
        }
        void SubscribeToLobbyInvites() { // Based on SDK sample's FLobbies::SubscribeToLobbyInvites
            // Sanity checks
            if (!IsInitialized()) {
                mLogger.AddErrorNetLog("Called but not yet initialized!");
                return;
            }
            if (IsSubscribedToLobbyInvites()) {
                mLogger.AddErrorNetLog("Already subscribed to connection requests");
                return;
            }
            
            EOS_HLobby lobbyHandle = EOS_Platform_GetLobbyInterface(mPlatformHandle); // Not using cached handle atm as that may not be set already (init flow vs login flow atm)

            EOS_Lobby_AddNotifyLobbyInviteReceivedOptions inviteOptions = {};
            inviteOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYINVITERECEIVED_API_LATEST;
            mLobbyInviteNotification = EOS_Lobby_AddNotifyLobbyInviteReceived(lobbyHandle, &inviteOptions, nullptr, 
                [](const EOS_Lobby_LobbyInviteReceivedCallbackInfo* data) {
                    Singleton<EOSWrapperSingleton>::get().OnLobbyInviteReceived(data);
                });

            EOS_Lobby_AddNotifyLobbyInviteAcceptedOptions acceptedOptions = {};
            acceptedOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYINVITEACCEPTED_API_LATEST;
            mLobbyInviteAcceptedNotification = EOS_Lobby_AddNotifyLobbyInviteAccepted(lobbyHandle, &acceptedOptions, nullptr, 
                [](const EOS_Lobby_LobbyInviteAcceptedCallbackInfo* data) {
                    Singleton<EOSWrapperSingleton>::get().OnLobbyInviteAccepted(data);
                });

            EOS_Lobby_AddNotifyJoinLobbyAcceptedOptions joinGameOptions = {};
            joinGameOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYJOINLOBBYACCEPTED_API_LATEST;
            mJoinLobbyAcceptedNotification = EOS_Lobby_AddNotifyJoinLobbyAccepted(lobbyHandle, &joinGameOptions, nullptr, 
                [](const EOS_Lobby_JoinLobbyAcceptedCallbackInfo* data) {
                    Singleton<EOSWrapperSingleton>::get().OnJoinLobbyAccepted(data);
                });

            // Success vs failure logging
            if (IsSubscribedToLobbyInvites()) {
                mLogger.AddInfoNetLog("Successfully subscribed");
            }
            else {
                mLogger.AddErrorNetLog("Failed to subscribe, bad notification id returned");
            }
        }
        void UnsubscribeFromLobbyInvites()
        {
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Called while not currently initialized");
                return;
            }
            if (!IsSubscribedToConnectionRequests()) {
                mLogger.AddWarnNetLog("Not subscribed already");
                return;
            }
            
            EOS_HLobby lobbyHandle = EOS_Platform_GetLobbyInterface(mPlatformHandle); // Not using cached handle atm as that may not be set already (init flow vs login flow atm)

            EOS_Lobby_RemoveNotifyLobbyInviteReceived(lobbyHandle, mLobbyInviteNotification);
            mLobbyInviteNotification = EOS_INVALID_NOTIFICATIONID;

            EOS_Lobby_RemoveNotifyLobbyInviteAccepted(lobbyHandle, mLobbyInviteAcceptedNotification);
            mLobbyInviteAcceptedNotification = EOS_INVALID_NOTIFICATIONID;

            EOS_Lobby_RemoveNotifyJoinLobbyAccepted(lobbyHandle, mJoinLobbyAcceptedNotification);
            mJoinLobbyAcceptedNotification = EOS_INVALID_NOTIFICATIONID;
        }

        bool IsSubscribedToLobbyUpdates() {
            return mLobbyUpdateNotification != EOS_INVALID_NOTIFICATIONID
                && mLobbyMemberUpdateNotification != EOS_INVALID_NOTIFICATIONID
                && mLobbyMemberStatusNotification != EOS_INVALID_NOTIFICATIONID;
        }
        void SubscribeToLobbyUpdates() { // Based on SDK sample's FLobbies::SubscribeToLobbyUpdates
            // Sanity checks
            if (!IsInitialized()) {
                mLogger.AddErrorNetLog("Called but not yet initialized!");
                return;
            }
            if (IsSubscribedToLobbyUpdates()) {
                mLogger.AddErrorNetLog("Already subscribed to connection requests");
                return;
            }
            
            EOS_HLobby lobbyHandle = EOS_Platform_GetLobbyInterface(mPlatformHandle); // Not using cached handle atm as that may not be set already (init flow vs login flow atm)

            EOS_Lobby_AddNotifyLobbyUpdateReceivedOptions updateNotifyOptions = {};
            updateNotifyOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYUPDATERECEIVED_API_LATEST;

            mLobbyUpdateNotification = EOS_Lobby_AddNotifyLobbyUpdateReceived(lobbyHandle, &updateNotifyOptions, nullptr, 
            [](const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnLobbyUpdateReceived(data);
            });

            EOS_Lobby_AddNotifyLobbyMemberUpdateReceivedOptions memberUpdateOptions = {};
            memberUpdateOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERUPDATERECEIVED_API_LATEST;
            mLobbyMemberUpdateNotification = EOS_Lobby_AddNotifyLobbyMemberUpdateReceived(lobbyHandle, &memberUpdateOptions, nullptr, 
            [](const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnMemberUpdateReceived(data);
            });

            EOS_Lobby_AddNotifyLobbyMemberStatusReceivedOptions memberStatusOptions = {};
            memberStatusOptions.ApiVersion = EOS_LOBBY_ADDNOTIFYLOBBYMEMBERSTATUSRECEIVED_API_LATEST;
            mLobbyMemberStatusNotification = EOS_Lobby_AddNotifyLobbyMemberStatusReceived(lobbyHandle, &memberStatusOptions, nullptr, 
            [](const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnMemberStatusReceived(data);
            });

            // Success vs failure logging
            if (IsSubscribedToLobbyUpdates()) {
                mLogger.AddInfoNetLog("Successfully subscribed");
            }
            else {
                mLogger.AddErrorNetLog("Failed to subscribe, bad notification id returned");
            }
        }
        void UnsubscribeToLobbyUpdates() {
            if (!IsInitialized()) {
                mLogger.AddWarnNetLog("Called while not currently initialized");
                return;
            }
            if (!IsSubscribedToLobbyUpdates()) {
                mLogger.AddWarnNetLog("Not subscribed already");
                return;
            }

            EOS_HLobby lobbyHandle = EOS_Platform_GetLobbyInterface(mPlatformHandle); // Not using cached handle atm as that may not be set already (init flow vs login flow atm)

            EOS_Lobby_RemoveNotifyLobbyUpdateReceived(lobbyHandle, mLobbyUpdateNotification);
            mLobbyUpdateNotification = EOS_INVALID_NOTIFICATIONID;

            EOS_Lobby_RemoveNotifyLobbyMemberUpdateReceived(lobbyHandle, mLobbyMemberUpdateNotification);
            mLobbyMemberUpdateNotification = EOS_INVALID_NOTIFICATIONID;

            EOS_Lobby_RemoveNotifyLobbyMemberStatusReceived(lobbyHandle, mLobbyMemberStatusNotification);
            mLobbyMemberStatusNotification = EOS_INVALID_NOTIFICATIONID;
        }
        #pragma endregion


        
        void HandlePendingUserInfoRequests() {
            if (!IsInitialized()) { // Sanity check
                mLogger.AddWarnNetLog("Not initialized");
                return;
            }

            // If not logged in, then currently not sending requests as don't see a need.
            //      Not *strictly* necessary for APIs but currently simply using logged in id for "LocalUserId" API params.
            if (!IsFullyLoggedIn()) {
                return;
            }
            
            // If no queries to send, then nothing to do here
            //      Not really necessary to check and stop ahead of time with current implement but like this approach
            if (!mPlayersInfoTracking.HasAnyNotSentQueries()) {
                return;
            }

            // Go through and send any not-yet-sent queries
            SendAccountMappingQueries(mPlayersInfoTracking.GetNotYetSentAccountMappingQueries());
            SendUserInfoQueries(mPlayersInfoTracking.GetNotYetSentUserInfoQueries());

            // Let underlying tracking know that all not-yet-sent queries have been sent.
            //      Helps to prevent sending needless duplicate queries.
            mPlayersInfoTracking.MarkAllNotYetSentQueriesAsSent();
        }
        // Based on SDK sample's FUsers::QueryAccountMappings
        void SendAccountMappingQueries(const std::set<CrossPlatformIdWrapper>& accountMappingRequests) {
            // Make sure input is not empty before doing any further work.
            //      Supposedly using a foreach iterator over an empty set may cause a crash, see relevant
            //      comments in OnQueryAccountMappingsResult()
            if (accountMappingRequests.empty()) {
                return;
            }

            // Convert inputs to array of direct ids, as SDK API supports a single query for entire vector
            std::vector<EOS_ProductUserId> requestRawIds = {};
            for (const CrossPlatformIdWrapper& crossPlatformIdWrapper : accountMappingRequests) {
                requestRawIds.push_back(crossPlatformIdWrapper.GetAccountId());
            }

            // Prep the SDK request struct
            EOS_Connect_QueryProductUserIdMappingsOptions QueryOptions = {};
            QueryOptions.ApiVersion = EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST;
            QueryOptions.LocalUserId = mLoggedInCrossPlatformId.GetAccountId();
            QueryOptions.ProductUserIdCount = static_cast<uint32_t>(requestRawIds.size());
            QueryOptions.ProductUserIds = requestRawIds.data();

            // Actually send the request
            auto simpleCallback = [](const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnQueryAccountMappingsResult(data);
            };
            EOS_HConnect connectHandle = EOS_Platform_GetConnectInterface(mPlatformHandle);
            EOS_Connect_QueryProductUserIdMappings(connectHandle, &QueryOptions, nullptr, simpleCallback);

            // Give a bit of feedback in logs to help keep track through full process of this info retrieval system
            mLogger.AddInfoNetLog("Sent queries for following # of ids: " + std::to_string(accountMappingRequests.size()));
        }
        void SendUserInfoQueries(const std::set<EpicAccountIdWrapper>& userInfoRequests) {
            // Make sure input is not empty before doing any further work.
            //      Supposedly using a foreach iterator over an empty set may cause a crash, see relevant
            //      comments in OnQueryAccountMappingsResult()
            if (userInfoRequests.empty()) {
                return;
            }

            // Bit of shared work between for loop iterations
            EOS_HUserInfo UserInfoInterface = EOS_Platform_GetUserInfoInterface(mPlatformHandle);
            auto simpleCallback = [](const EOS_UserInfo_QueryUserInfoCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnQueryUserInfoResult(data);
            };

            // Actually send a query per request
            for (const EpicAccountIdWrapper& epicAccountId : userInfoRequests) {
                EOS_UserInfo_QueryUserInfoOptions QueryUserInfoOptions = {};
                QueryUserInfoOptions.ApiVersion = EOS_USERINFO_QUERYUSERINFO_API_LATEST;
                QueryUserInfoOptions.LocalUserId = mLoggedInEpicAccountId.GetAccountId();
                QueryUserInfoOptions.TargetUserId = epicAccountId.GetAccountId();

                EOS_UserInfo_QueryUserInfo(UserInfoInterface, &QueryUserInfoOptions, nullptr, simpleCallback);
            }

            // Give a bit of feedback in logs to help keep track through full process of this info retrieval system
            mLogger.AddInfoNetLog("Sent queries for following # of ids: " + std::to_string(userInfoRequests.size()));
        }


        
        bool IsInitialized() const {
            return mIsInitialized;
        }

        // FUTURE: Clean up the excessive # of login check methods
        bool IsLoggedInAtAll() { // Not sure if "Basic" login even matters much, but staying on safe side with this method
            return IsBasicLoggedIn() || IsCrossPlatformLoggedIn();
        }
        bool IsFullyLoggedIn() { // Nice little wrapper to be explicit for what the cross platform login means
            return IsCrossPlatformLoggedIn();
        }
        bool IsBasicLoggedIn() {
            return mLoggedInEpicAccountId.IsValid();
        }
        bool IsCrossPlatformLoggedIn() {
            return mLoggedInCrossPlatformId.IsValid();
        }

        const std::string kTestSocketName = "TEST_SOCKET";
        // Below comment from SessionMatchmaking.cpp in SDK Samples:
        //      The top - level, game - specific filtering information for session searches. This criteria should be set with mostly static, coarse settings, often formatted like "GameMode:Region:MapName".
        static constexpr char kDefaultGameLobbyBucketId[] = "TODO:GameMode:Region";
        static constexpr bool kIsLobbyPresenceEnabled = true; // Assuming no need for private presence
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();
        bool mIsInitialized = false;

        IEOSWrapperManager* mEosWrapperManager = nullptr;

        // Current user state
        EpicAccountIdWrapper mLoggedInEpicAccountId = {};
        CrossPlatformIdWrapper mLoggedInCrossPlatformId = {};
        EOSPlayersInfoTracking mPlayersInfoTracking = {};
        EOSLobbyTracking mMainMatchLobby = {};

        // Various EOS handles
        EOS_HPlatform mPlatformHandle = nullptr;
        // Note that the below handles depend on logged in state given current implementation
        //      ie, haven't yet tested if can retrieve these before logging in.
        EOS_HAuth mAuthHandle = nullptr;
        EOS_HConnect mConnectHandle = nullptr;
        EOS_HUI mUIHandle = nullptr;
        EOS_HLobby mLobbyHandle = nullptr;

        // Id used with managing incoming connection notifications. Not the same as a socket id or such
        EOS_NotificationId mP2PConnectionRequestNotificationId = EOS_INVALID_NOTIFICATIONID;
        EOS_NotificationId mP2PConnectionEstablishedNotificationId = EOS_INVALID_NOTIFICATIONID;
        EOS_NotificationId mP2PConnectionInterruptedNotificationId = EOS_INVALID_NOTIFICATIONID;
        EOS_NotificationId mP2PConnectionClosedNotificationId = EOS_INVALID_NOTIFICATIONID;
        EOS_NotificationId mP2PIncomingPacketQueueFullNotificationId = EOS_INVALID_NOTIFICATIONID;
        // Lobby general update notifications
        EOS_NotificationId mLobbyUpdateNotification = EOS_INVALID_NOTIFICATIONID;
        EOS_NotificationId mLobbyMemberUpdateNotification = EOS_INVALID_NOTIFICATIONID;
        EOS_NotificationId mLobbyMemberStatusNotification = EOS_INVALID_NOTIFICATIONID;
        // Lobby invite-related notifications
        EOS_NotificationId mLobbyInviteNotification = EOS_INVALID_NOTIFICATIONID;
        EOS_NotificationId mLobbyInviteAcceptedNotification = EOS_INVALID_NOTIFICATIONID;
        EOS_NotificationId mJoinLobbyAcceptedNotification = EOS_INVALID_NOTIFICATIONID;
    };
}
