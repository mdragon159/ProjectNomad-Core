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
#include "NetLobbySlot.h"
#include "NetLobbyTracking.h"
#include "PacketReliability.h"
#include "EOS/Include/eos_lobby.h"
#include "EOS/Include/eos_ui.h"
#include "Network/NetworkManagerCallbackTypes.h"
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
                mLogger.addWarnNetLog("EWS::Initialize", "NetworkManager is already initialized");
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
                mLogger.addInfoNetLog(
                    "EWS::Initialize",
                    "EOS_Initialize returned EOS_AlreadyConfigured, proceeding with init attempt"
                );
            } else if (initResult != EOS_EResult::EOS_Success) {
                mLogger.addErrorNetLog(
                    "EWS::Initialize",
                    "EOS SDK failed to init with result: " + std::string(EOS_EResult_ToString(initResult))
                );
                return false;
            }

            mLogger.addInfoNetLog("EWS::Initialize", "EOS SDK initialized, setting logging callback...");
            EOS_EResult setLogCallbackResult = EOS_Logging_SetCallback([](const EOS_LogMessage* message) {
                Singleton<EOSWrapperSingleton>::get().HandleEosLogMessage(message);
            });
            if (setLogCallbackResult != EOS_EResult::EOS_Success) {
                mLogger.addWarnNetLog("EWS::Initialize", "Set logging callback failed");
                return false;
            } 

            mLogger.addInfoNetLog("EWS::Initialize", "Logging callback set");
            EOS_Logging_SetLogLevel(EOS_ELogCategory::EOS_LC_ALL_CATEGORIES, EOS_ELogLevel::EOS_LOG_Verbose);

            if (TryCreatePlatformInstance()) {
                SetNetStatus(EOSWrapperStatus::Initialized);
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
            
            // Clean up callbacks to assure they don't persist between game instances/runs
            mNetStatusChangeCallback = nullptr;
            mNetGotSelfIdCallback = nullptr;
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
                    mCurrentNetStatus = EOSWrapperStatus::NotInitialized;
                    mLogger.addInfoNetLog("EWS::Shutdown", "Shutdown attempt completed");
                }
                else {
                    mLogger.addWarnNetLog("EWS::Shutdown", "Not initialized");
                }
            }
        }

        // Normal update per frame
        void Tick() {
            if (mPlatformHandle) {
                EOS_Platform_Tick(mPlatformHandle);
            }

            HandleReceivedMessages();
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
                mLogger.addWarnNetLog("EWS::Logout", "Not logged in");
                return false;
            }
            if (mAuthHandle == nullptr) {
                mLogger.addErrorNetLog("EWS::Logout", "Is logged in but somehow no auth handle");
                return false;
            }

            mLogger.addInfoNetLog("EWS::Logout", "Logging out...");

            EOS_Auth_LogoutOptions LogoutOptions;
            LogoutOptions.ApiVersion = EOS_AUTH_LOGOUT_API_LATEST;
            LogoutOptions.LocalUserId = mLoggedInEpicAccountId.getAccountId();

            EOS_Auth_Logout(mAuthHandle, &LogoutOptions, nullptr, [](const EOS_Auth_LogoutCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnLogoutComplete(data);
            });
            return true;
        }
#pragma endregion

#pragma region Callback Registration
        // TODO: Delete these old listeners
        void RegisterNetStatusChangeListener(const NetStatusChangeCallback& callback) {
            mNetStatusChangeCallback = callback;

            // Immediately call the callback as a workaround to Singleton being long-living (eg, already being initialized on editor play start)
            mNetStatusChangeCallback(mCurrentNetStatus);
        }

        void RegisterNetGotSelfIdListener(const NetGotSelfIdCallback& callback) {
            mNetGotSelfIdCallback = callback;

            // Not expecting id to already be set in reality, but just in case- if id is somehow set, then call callback immediately
            if (mLoggedInCrossPlatformId.isValid()) {
                std::string result;
                if (!mLoggedInCrossPlatformId.tryToString(result)) {
                    mLogger.addErrorNetLog("EWS::registerNetGotSelfIdListener", "Failed to convert id to string");
                    return;
                }

                callback(result);
            }
        }

        void SetWrapperManager(IEOSWrapperManager* iEOSWrapperManager) {
            mEosWrapperManager = iEOSWrapperManager;
        }
#pragma endregion

#pragma region Ordinary Connection & Message Sending
        CrossPlatformIdWrapper ConvertToCrossPlatformIdType(const std::string& targetCrossPlatformId) {
            EOS_ProductUserId formattedTargetCrossPlatformId = EOS_ProductUserId_FromString(targetCrossPlatformId.c_str());
            return CrossPlatformIdWrapper(formattedTargetCrossPlatformId);
        }

        void SendMessage(CrossPlatformIdWrapper targetId, const void* data, uint32_t dataLengthInBytes, PacketReliability packetReliability) {
            if (!IsCrossPlatformLoggedIn()) {
                mLogger.addWarnNetLog("EWS::sendMessage", "Not cross platform logged in");
                return;
            }
            if (!targetId.isValid()) {
                mLogger.addWarnNetLog("EWS::sendMessage", "Target cross platform id is invalid");
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);

            EOS_P2P_SocketId socketId;
            socketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
            strncpy_s(socketId.SocketName, "CHAT", 5);

            EOS_P2P_SendPacketOptions options;
            options.ApiVersion = EOS_P2P_SENDPACKET_API_LATEST;
            options.LocalUserId = mLoggedInCrossPlatformId.getAccountId();
            options.RemoteUserId = targetId.getAccountId();
            options.SocketId = &socketId;
            options.bAllowDelayedDelivery = EOS_TRUE; // PLACEHOLDER: Set to false once separately setting up connections
            options.Channel = 0;
            options.Reliability = ConvertPacketReliability(packetReliability);

            options.DataLengthBytes = dataLengthInBytes;
            options.Data = data;

            EOS_EResult result = EOS_P2P_SendPacket(p2pHandle, &options);
            if (result == EOS_EResult::EOS_Success) {
                // FUTURE: Return whether or not sending the message was a success
                
                // logger.addInfoNetLog(
                //     "EWS::sendMessage",
                //     "Successfully attempted to send message"
                // );
            }
            if (result != EOS_EResult::EOS_Success) {
                mLogger.addErrorNetLog(
                    "EWS::sendMessage",
                    "Failed to send message");
            }
        }
#pragma endregion

#pragma region Lobby & Sessions
        bool BeginCreateMainMatchLobby() {
            // Specific lobby type sanity check
            if (!mMainMatchLobby.IsCompletelyInactive()) {
                mLogger.addWarnNetLog(
                    "EWS::BeginCreateLobby",
                    "Main lobby not entirely inactive. Current status: " + mMainMatchLobby.GetCurrentStatusAsNumber()
                );
                return false;
            }

            bool didSuccessfullyStartLobbyCreation = BeginCreateLobby();
            if (didSuccessfullyStartLobbyCreation) {
                // Track lobby creation for general validation purposes
                mMainMatchLobby.SetBeingCreatedState(mLogger);
            }

            return didSuccessfullyStartLobbyCreation;
        }

        bool BeginLeaveMainGameLobby() {
            return BeginLeaveLobby(NetLobbySlot::MainMatchLobby);
        }
#pragma endregion

#pragma region "Other" Bucket of Utility Methods
        void ShowFriendsUI() {
            if (!IsInitialized()) {
                mLogger.addWarnNetLog("EWS::ShowFriendsUI", "Not initialized");
                return;
            }
            if (!IsLoggedInAtAll()) {
                mLogger.addWarnNetLog("EWS::ShowFriendsUI", "Not logged in yet");
                return;
            }
            if (mUIHandle == nullptr) {
                mLogger.addErrorNetLog("EWS::ShowFriendsUI", "mUIHandle null somehow");
                return;
            }

            // Based on UE OnlineSubsystemEOS plugin's UserManagerEOS::ShowFriendsUI
            EOS_UI_ShowFriendsOptions options = {};
            options.ApiVersion = EOS_UI_SHOWFRIENDS_API_LATEST;
            options.LocalUserId = mLoggedInEpicAccountId.getAccountId();

            auto passthroughCallback = [](const EOS_UI_ShowFriendsCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnShowFriendsResult(data); // See 
            };
            EOS_UI_ShowFriends(mUIHandle, &options, nullptr, passthroughCallback);
        }
#pragma endregion 

#pragma region Debug/Testing Methods
        void TestSendMessage(const std::string& targetCrossPlatformId, const std::string& message) {
            if (targetCrossPlatformId.empty()) {
                mLogger.addWarnNetLog("EWS::TestSendMessage", "Empty targetCrossPlatformId");
                return;
            }
            if (message.empty()) {
                mLogger.addWarnNetLog("EWS::TestSendMessage", "Empty message, nothing to send");
                return;
            }

            SendMessage(
                CrossPlatformIdWrapper::fromString(targetCrossPlatformId),
                message.data(),
                static_cast<uint32_t>(message.size()),
                PacketReliability::ReliableOrdered
            );            
        }

        void PrintLoggedInId() {
            if (!IsBasicLoggedIn()) {
                mLogger.addWarnNetLog("EWS::PrintLoggedInId", "Not logged in");
                return;
            }

            std::string idAsString;
            if (!mLoggedInEpicAccountId.tryToString(idAsString)) {
                mLogger.addErrorNetLog("EWS::PrintLoggedInId", "Failed to convert id to string");
                return;
            }

            mLogger.addInfoNetLog("EWS::PrintLoggedInId", "Id: " + idAsString);
        }

        void PrintLoggedInCrossPlatformId() {
            if (!IsCrossPlatformLoggedIn()) {
                mLogger.addWarnNetLog("EWS::PrintLoggedInCrossPlatformId", "Not logged in");
                return;
            }

            std::string idAsString;
            if (!mLoggedInCrossPlatformId.tryToString(idAsString)) {
                mLogger.addErrorNetLog("EWS::PrintLoggedInCrossPlatformId", "Failed to convert id to string");
                return;
            }

            mLogger.addInfoNetLog("EWS::PrintLoggedInCrossPlatformId", "Id: " + idAsString);
        }
#pragma endregion

#pragma region EOS Callbacks
        void HandleEosLogMessage(const EOS_LogMessage* message) {
            std::string identifier = "[EOS SDK] " + std::string(message->Category);

            switch (message->Level) {
            case EOS_ELogLevel::EOS_LOG_Warning:
                mLogger.addWarnNetLog(identifier, message->Message, NetLogCategory::EosSdk);
                break;

            case EOS_ELogLevel::EOS_LOG_Error:
            case EOS_ELogLevel::EOS_LOG_Fatal:
                mLogger.addErrorNetLog(identifier, message->Message, NetLogCategory::EosSdk);
                break;

            default:
                mLogger.addInfoNetLog(identifier, message->Message, NetLogCategory::EosSdk);
            }
        }

        // Based on SDK sample's FAuthentication::LoginCompleteCallbackFn
        void OnLoginComplete(const EOS_Auth_LoginCallbackInfo* data) {
            if (data == nullptr) {
                mLogger.addErrorNetLog("EWS::LoginCompleteCallback", "Input data is unexpectedly null");
                return;
            }

            if (data->ResultCode == EOS_EResult::EOS_Success) {
                OnSuccessfulEpicLogin(data->LocalUserId);
            }
            else if (data->ResultCode == EOS_EResult::EOS_Auth_PinGrantCode) {
                mLogger.addWarnNetLog("EWS::LoginCompleteCallback", "Waiting for PIN grant...?");
            }
            else if (data->ResultCode == EOS_EResult::EOS_Auth_MFARequired) {
                mLogger.addWarnNetLog(
                    "EWS::LoginCompleteCallback",
                    "MFA Code needs to be entered before logging in"
                );

                // PLACEHOLDER: Callback or such to handle this case
                // See SDK sample relevant code (event type UserLoginRequiresMFA). Eg:
                // FGameEvent Event(EGameEventType::UserLoginRequiresMFA, Data->LocalUserId);
            }
            else if (data->ResultCode == EOS_EResult::EOS_InvalidUser) {
                if (data->ContinuanceToken != nullptr) {
                    mLogger.addWarnNetLog("EWS::LoginCompleteCallback", "Login failed, external account not found");

                    // PLACEHOLDER: Check sample's FAuthentication::LoginCompleteCallbackFn for relevant code here
                    // Something something try to continue login or something...?
                    
                    // See following section in docs for more info:
                    // https://dev.epicgames.com/docs/services/en-US/EpicAccountServices/AuthInterface/index.html#externalaccountauthentication
                } else {
                    mLogger.addErrorNetLog("EWS::LoginCompleteCallback", "Continuation Token is invalid");
                }
            }
            else if (data->ResultCode == EOS_EResult::EOS_Auth_AccountFeatureRestricted) {
                if (data->AccountFeatureRestrictedInfo) {
                    std::string verificationURI = data->AccountFeatureRestrictedInfo->VerificationURI;
                    mLogger.addErrorNetLog(
                        "EWS::LoginCompleteCallback",
                        "Login failed, account is restricted. User must visit URI: " + verificationURI
                    );
                } else {
                    mLogger.addErrorNetLog(
                        "EWS::LoginCompleteCallback",
                        "Login failed, account is restricted. VerificationURI is invalid!"
                    );
                }
            }
            else if (EOS_EResult_IsOperationComplete(data->ResultCode)) {
                mLogger.addErrorNetLog(
                    "EWS::LoginCompleteCallback",
                    "Login Failed - Error Code: " + ResultCodeToString(data->ResultCode)
                );
            }
            else {
                mLogger.addWarnNetLog("EWS::LoginCompleteCallback", "Hit final else statement unexpectedly");
            }
        }

        void OnCrossPlatformLoginComplete(const EOS_Connect_LoginCallbackInfo* data) {
            if (data == nullptr) {
                mLogger.addErrorNetLog("EWS::CrossPlatformLoginCompleteCallback", "Data is nullptr");
                return;
            }

            if (data->ResultCode == EOS_EResult::EOS_Success) {
                OnSuccessfulCrossPlatformLogin(data->LocalUserId);
            }
            else {
                mLogger.addErrorNetLog(
                    "EWS::CrossPlatformLoginCompleteCallback",
                    "Cross plat login failed with result: " + ResultCodeToString(data->ResultCode));
            }
        }

        void OnLogoutComplete(const EOS_Auth_LogoutCallbackInfo* data) {
            if (data == nullptr) {
                mLogger.addErrorNetLog("EWS::LogoutCompleteCallback", "Input data is unexpectedly null");
                return;
            }

            if (data->ResultCode == EOS_EResult::EOS_Success) {
                mLogger.addInfoNetLog("EWS::LogoutCompleteCallback", "Logged out successfully");
                if (mEosWrapperManager) {
                    mEosWrapperManager->OnLogoutSuccess();
                }

                // Note that logging out event game-wise can (and likely should be) handled on the LoginStatusChanged callback side
            }
            else {
                mLogger.addErrorNetLog(
                    "EWS::LogoutCompleteCallback",
                    "Logout Failed - Error Code: " + ResultCodeToString(data->ResultCode)
                );
            }

            // Arbitrarily assuming that not logged in regardless of what result occurred. Thus resetting related data
            // (Note that in reality there COULD be race conditions with these callbacks depending on how the SDK is implemented)
            // (Thus this may burn us in the future)
            // TODO: Reset session, lobby, etc data just in case. (Not yet implemented as really should clean those up BEFORE fully logging out)
            ResetLoginData();
        }

        // Based on SDK sample's FP2PNAT::OnIncomingConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* Data)
        void OnIncomingConnectionRequest(const EOS_P2P_OnIncomingConnectionRequestInfo* data) {
            if (data == nullptr) {
                mLogger.addErrorNetLog("EWS::OnIncomingConnectionRequest", "Input data is unexpectedly null");
                return;
            }

            if (!IsCrossPlatformLoggedIn()) {
                mLogger.addErrorNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "Somehow receiving connection requests without being cross platform logged in. Perhaps outdated expectation?");
                return;
            }

            std::string socketName = data->SocketId->SocketName;
            if (socketName != "CHAT") {
                mLogger.addWarnNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "Unexpected socket name, ignoring. Incoming name: " + socketName);
                return;
            }

            // Accepting connection based on SDK sample's FP2PNAT::OnIncomingConnectionRequest
            
            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);
            EOS_P2P_AcceptConnectionOptions options;
            options.ApiVersion = EOS_P2P_ACCEPTCONNECTION_API_LATEST;
            options.LocalUserId = mLoggedInCrossPlatformId.getAccountId();
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
                    mLogger.addWarnNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "Could not convert remote user id to string"
                    );
                    return;
                }
                
                mLogger.addInfoNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "Successfully accepted connection from: " + remoteIdAsString
                );
            }
            else {
                mLogger.addErrorNetLog(
                    "EWS::OnIncomingConnectionRequest",
                    "EOS_P2P_AcceptConnection failed with result: " + ResultCodeToString(result)
                );
            }
        }

        void OnShowFriendsResult(const EOS_UI_ShowFriendsCallbackInfo* data) {
            if (data->ResultCode == EOS_EResult::EOS_Success) {
                mLogger.addInfoNetLog("EWS::OnShowFriendsCallback",  "Success!");
            }
            else {
                mLogger.addWarnNetLog(
                    "EWS::OnShowFriendsCallback",
                    "Failed with result code: " + ResultCodeToString(data->ResultCode)
                );
            }
        }

        // Based on SDK sample's FLobbies::OnCreateLobbyFinished
        void OnCreateLobbyFinished(const EOS_Lobby_CreateLobbyCallbackInfo* data) {
            if (!data) {
                mLogger.addWarnNetLog("EWS::OnCreateLobbyFinished", "Input data is somehow null!");
                mEosWrapperManager->OnLobbyCreationResult(false);
                return;
            }
            
            if (!EOS_EResult_IsOperationComplete(data->ResultCode)) {
                mLogger.addWarnNetLog(
                    "EWS::OnCreateLobbyFinished",
                    "Operation not complete, should be automatically called again in near future. Result code: "
                    + ResultCodeToString(data->ResultCode)
                );
                return; // For now just wait until completion
            }

            // Handle actual result
            bool didSetupSucceed = false;
            if (data->ResultCode != EOS_EResult::EOS_Success) {
                mLogger.addWarnNetLog(
                    "EWS::OnCreateLobbyFinished",
                    "Creation failed with error code: " + ResultCodeToString(data->ResultCode)
                );
                didSetupSucceed = false; // Nice to be explicit
            }
            else {
                mLogger.addInfoNetLog("EWS::OnCreateLobbyFinished", "Lobby successfully created!");

                // Try to setup lobby info tracking
                didSetupSucceed = mMainMatchLobby.TryInitActiveLobby(mLogger, data->LobbyId);
            }

            // If didn't succeed, then make sure to reset tracking so no problems with trying to create future lobbies
            if (!didSetupSucceed) {
                mMainMatchLobby.ResetLobbyInfo();
            }

            // Finally let rest of game handle lobby creation as appropriate
            if (mEosWrapperManager) {
                mEosWrapperManager->OnLobbyCreationResult(didSetupSucceed);
            }
        }

        // Based on SDK sample's FLobbies::OnLeaveLobbyFinished
        void OnLeaveLobbyFinished(const EOS_Lobby_LeaveLobbyCallbackInfo* data) {
            if (!data) {
                mLogger.addWarnNetLog("EWS::OnLeaveLobbyFinished", "Input data is somehow null!");
                mEosWrapperManager->OnLobbyLeftOrDestroyed(false);
                return;
            }

            if (!EOS_EResult_IsOperationComplete(data->ResultCode)) {
                mLogger.addWarnNetLog(
                    "EWS::OnLeaveLobbyFinished",
                    "Operation not complete, should be automatically called again in near future. Result code: "
                    + ResultCodeToString(data->ResultCode)
                );
                return; // For now just wait until completion
            }

            // Handle actual result
            bool didLeaveSucceed = false;
            if (data->ResultCode != EOS_EResult::EOS_Success) {
                mLogger.addWarnNetLog(
                    "EWS::OnLeaveLobbyFinished",
                    "Leave failed with error code: " + ResultCodeToString(data->ResultCode)
                );
                didLeaveSucceed = false; // Nice to be explicit
            }
            else {
                mLogger.addInfoNetLog("EWS::OnLeaveLobbyFinished", "Successfully left lobby!");
                didLeaveSucceed = true;

                // Reset lobby so we can reuse it going forward (ie, create a new lobby without validation issues)
                NetLobbySlot netLobbySlot = FakePointerToLobbySlot(data->ClientData);
                NetLobbyTracking& netLobbyTracking = GetLobbyInfoForSlot(netLobbySlot);
                netLobbyTracking.ResetLobbyInfo();
            }

            // Note: No idea what should do atm if leaving lobby fails. Need to see an actual case in wild (or docs) methinks.
            //       Likely just have a backup "force leave" option for frontend at least.

            if (mEosWrapperManager) {
                mEosWrapperManager->OnLobbyLeftOrDestroyed(didLeaveSucceed);
            }
        }

        void OnLobbyInviteReceived(const EOS_Lobby_LobbyInviteReceivedCallbackInfo* data) {
            if (!data) {
                mLogger.addWarnNetLog("EWS::OnLobbyInviteReceived", "Input data is somehow null!");
                return;
            }
        }
        void OnLobbyInviteAccepted(const EOS_Lobby_LobbyInviteAcceptedCallbackInfo* data) {
            if (!data) {
                mLogger.addWarnNetLog("EWS::OnLobbyInviteAccepted", "Input data is somehow null!");
                return;
            }

            // 1. Get lobby info (lotsa pain)
            // 2. Do shared logic for joining lobby
        }
        void OnJoinLobbyAccepted(const EOS_Lobby_JoinLobbyAcceptedCallbackInfo* data) {
            if (!data) {
                mLogger.addWarnNetLog("EWS::OnJoinLobbyAccepted", "Input data is somehow null!");
                return;
            }

            // 1. Get lobby info (lotsa pain)
            // 2. Do shared logic for joining lobby
        }

        void OnLobbyUpdateReceived(const EOS_Lobby_LobbyUpdateReceivedCallbackInfo* data) {
            if (!data) {
                mLogger.addWarnNetLog("EWS::OnLobbyUpdateReceived", "Input data is somehow null!");
                return;
            }

            // Shared logic: OnLobbyUpdate w/ lobby id
        }
        void OnMemberUpdateReceived(const EOS_Lobby_LobbyMemberUpdateReceivedCallbackInfo* data) {
            if (!data) {
                mLogger.addWarnNetLog("EWS::OnMemberUpdateReceived", "Input data is somehow null!");
                return;
            }

            // Shared logic: OnLobbyUpdate w/ lobby id
        }
        void OnMemberStatusReceived(const EOS_Lobby_LobbyMemberStatusReceivedCallbackInfo* data) {
            if (!data) {
                mLogger.addWarnNetLog("EWS::OnMemberStatusReceived", "Input data is somehow null!");
                return;
            }

            // TODO: If target user id == self AND current status == closed/kicked/disconnected, then handle as if kicked or lobby closed or such

            // Otherwise, do shared logic: OnLobbyUpdate w/ lobby id
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
                mLogger.addErrorNetLog("EWS::createPlatformInstance", "EOS Platform failed to init");
                return false;
            }

            mLogger.addInfoNetLog("EWS::createPlatformInstance", "EOS Platform successfully initialized!");
            return true;
        }

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
                mLogger.addWarnNetLog("EWS::BeginLoginAttempt", "Not initialized");
                return false;
            }
            if (IsLoggedInAtAll()) {
                mLogger.addWarnNetLog("EWS::BeginLoginAttempt", "Already logged in");
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
            mLogger.addInfoNetLog(
                "EWS::BeginLoginAttempt",
                "Successfully sent login request"
            );
            return true;
        }

        void OnSuccessfulEpicLogin(const EOS_EpicAccountId resultUserId) {
            mLoggedInEpicAccountId = EpicAccountIdWrapper(resultUserId);

            std::string userIdAsString;
            if (!mLoggedInEpicAccountId.tryToString(userIdAsString)) {
               mLogger.addErrorNetLog("EWS::onSuccessfulLogin", "Could not convert user id to string");
            }
            mLogger.addInfoNetLog(
                "EWS::onSuccessfulLogin",
                "Login Complete - User ID: " + userIdAsString
            );
            
            StartCrossPlatformLogin();
        }

        void OnSuccessfulCrossPlatformLogin(const EOS_ProductUserId resultCrossPlatformId) {
            mLoggedInCrossPlatformId = CrossPlatformIdWrapper(resultCrossPlatformId);
            SetNetStatus(EOSWrapperStatus::LoggedIn);

            std::string userIdAsString;
            if (!mLoggedInCrossPlatformId.tryToString(userIdAsString)) {
                mLogger.addErrorNetLog(
                    "EWS::onSuccessfulCrossPlatformLogin",
                    "Could not convert cross platform id to string");
            }
            mLogger.addInfoNetLog(
                "EWS::onSuccessfulCrossPlatformLogin",
                "Login Complete - Cross Platform User ID: " + userIdAsString
            );

            // Subscribe to various relevant updates that may happen during netcode usage
            SubscribeToConnectionRequests(); // Must be done after logging in, otherwise get EOS SDK log warnings/errors
            SubscribeToLobbyInvites();
            SubscribeToLobbyUpdates();

            if (mNetGotSelfIdCallback) {
                mNetGotSelfIdCallback(userIdAsString);
            }
            if (mEosWrapperManager) {
                mEosWrapperManager->OnLoginSuccess(mLoggedInCrossPlatformId);
            }
        }

        // Based on SDK sample's FP2PNAT::HandleReceivedMessages
        void HandleReceivedMessages() {
            if (!IsCrossPlatformLoggedIn()) {
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);

            EOS_P2P_ReceivePacketOptions options;
            options.ApiVersion = EOS_P2P_RECEIVEPACKET_API_LATEST;
            options.LocalUserId = mLoggedInCrossPlatformId.getAccountId();
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
                if (mEosWrapperManager) {
                    mEosWrapperManager->OnMessageReceived(CrossPlatformIdWrapper(peerId), messageData);
                }
            }
            else
            {
                mLogger.addErrorNetLog(
                    "EWS::handleReceivedMessages",
                    "EOS_P2P_ReceivePacket failed with result: " + ResultCodeToString(result)
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
        
        void ResetLobbiesAndSessionsTracking() {
            mMainMatchLobby.ResetLobbyInfo();
        }

        
        
        // Based on SDK sample's FAuthentication::ConnectLogin
        void StartCrossPlatformLogin() {
            // Sanity checks
            if (!IsBasicLoggedIn()) {
                mLogger.addErrorNetLog("EWS::StartCrossPlatformLogin", "Is not 'basic' logged in yet");
                return;
            }
            if (mAuthHandle == nullptr) {
                mLogger.addErrorNetLog("EWS::StartCrossPlatformLogin", "AuthHandle null");
                return;
            }
            if (mConnectHandle == nullptr) {
                mLogger.addErrorNetLog("EWS::StartCrossPlatformLogin", "ConnectHandle null");
                return;
            }
            
            EOS_Auth_Token* userAuthToken = nullptr;
            
            EOS_Auth_CopyUserAuthTokenOptions copyTokenOptions = { 0 };
            copyTokenOptions.ApiVersion = EOS_AUTH_COPYUSERAUTHTOKEN_API_LATEST;
            EOS_EResult result = EOS_Auth_CopyUserAuthToken(
                mAuthHandle, &copyTokenOptions, mLoggedInEpicAccountId.getAccountId(), &userAuthToken
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
                mLogger.addErrorNetLog(
                    "EWS::startCrossPlatformLogin",
                    "EOS_Auth_CopyUserAuthToken failed with result: " + ResultCodeToString(result)
                );
            }
        }


        bool IsSubscribedToConnectionRequests() {
            return mConnectionNotificationId != EOS_INVALID_NOTIFICATIONID;
        }
        void SubscribeToConnectionRequests() {
            // Sanity checks
            if (!IsInitialized()) {
                mLogger.addErrorNetLog(
                    "EWS::SubscribeToConnectionRequests",
                    "Called but not yet initialized!"
                );
                return;
            }
            if (IsSubscribedToConnectionRequests()) {
                mLogger.addErrorNetLog(
                    "EWS::SubscribeToConnectionRequests",
                    "Already subscribed to connection requests"
                );
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);

            EOS_P2P_SocketId socketId;
            socketId.ApiVersion = EOS_P2P_SOCKETID_API_LATEST;
            // strncpy_s(socketId.SocketName, TEST_SOCKET_NAME.c_str(), TEST_SOCKET_NAME.size() + 1);
            strncpy_s(socketId.SocketName, "CHAT", 5); // TODO: Why didn't above custom name setting work?

            EOS_P2P_AddNotifyPeerConnectionRequestOptions options;
            options.ApiVersion = EOS_P2P_ADDNOTIFYPEERCONNECTIONREQUEST_API_LATEST;
            options.LocalUserId = mLoggedInCrossPlatformId.getAccountId();
            options.SocketId = &socketId;

            mConnectionNotificationId = EOS_P2P_AddNotifyPeerConnectionRequest(p2pHandle, &options, nullptr,
                [](const EOS_P2P_OnIncomingConnectionRequestInfo* data) {
                    Singleton<EOSWrapperSingleton>::get().OnIncomingConnectionRequest(data);
                });

            // Success vs failure logging
            if (IsSubscribedToConnectionRequests()) {
                mLogger.addInfoNetLog(
                    "EWS::SubscribeToConnectionRequests",
                    "Successfully subscribed"
                );
            }
            else {
                mLogger.addErrorNetLog(
                    "EWS::SubscribeToConnectionRequests",
                    "Failed to subscribe, bad notification id returned"
                );
            }
        }
        void UnsubscribeFromConnectionRequests() {
            if (!IsInitialized()) {
                mLogger.addWarnNetLog("EWS::UnsubscribeFromConnectionRequests", "Called while not currently initialized");
                return;
            }
            if (!IsSubscribedToConnectionRequests()) {
                mLogger.addWarnNetLog("EWS::unsubscribeFromConnectionRequests", "Not subscribed already");
                return;
            }

            EOS_HP2P p2pHandle = EOS_Platform_GetP2PInterface(mPlatformHandle);
            EOS_P2P_RemoveNotifyPeerConnectionRequest(p2pHandle, mConnectionNotificationId);
            
            mConnectionNotificationId = EOS_INVALID_NOTIFICATIONID;
        }

        bool IsSubscribedToLobbyInvites() {
            return mLobbyInviteNotification != EOS_INVALID_NOTIFICATIONID
                && mLobbyInviteAcceptedNotification != EOS_INVALID_NOTIFICATIONID
                && mJoinLobbyAcceptedNotification != EOS_INVALID_NOTIFICATIONID;
        }
        void SubscribeToLobbyInvites() { // Based on SDK sample's FLobbies::SubscribeToLobbyInvites
            // Sanity checks
            if (!IsInitialized()) {
                mLogger.addErrorNetLog(
                    "EWS::SubscribeToLobbyInvites",
                    "Called but not yet initialized!"
                );
                return;
            }
            if (IsSubscribedToLobbyInvites()) {
                mLogger.addErrorNetLog(
                    "EWS::SubscribeToLobbyInvites",
                    "Already subscribed to connection requests"
                );
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
                mLogger.addInfoNetLog(
                    "EWS::SubscribeToLobbyInvites",
                    "Successfully subscribed"
                );
            }
            else {
                mLogger.addErrorNetLog(
                    "EWS::SubscribeToLobbyInvites",
                    "Failed to subscribe, bad notification id returned"
                );
            }
        }
        void UnsubscribeFromLobbyInvites()
        {
            if (!IsInitialized()) {
                mLogger.addWarnNetLog("EWS::UnsubscribeFromLobbyInvites", "Called while not currently initialized");
                return;
            }
            if (!IsSubscribedToConnectionRequests()) {
                mLogger.addWarnNetLog("EWS::IsSubscribedToConnectionRequests", "Not subscribed already");
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
                mLogger.addErrorNetLog(
                    "EWS::SubscribeToLobbyUpdates",
                    "Called but not yet initialized!"
                );
                return;
            }
            if (IsSubscribedToLobbyUpdates()) {
                mLogger.addErrorNetLog(
                    "EWS::SubscribeToLobbyUpdates",
                    "Already subscribed to connection requests"
                );
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
                mLogger.addInfoNetLog(
                    "EWS::SubscribeToLobbyUpdates",
                    "Successfully subscribed"
                );
            }
            else {
                mLogger.addErrorNetLog(
                    "EWS::SubscribeToLobbyUpdates",
                    "Failed to subscribe, bad notification id returned"
                );
            }
        }
        void UnsubscribeToLobbyUpdates() {
            if (!IsInitialized()) {
                mLogger.addWarnNetLog("EWS::UnsubscribeToLobbyUpdates", "Called while not currently initialized");
                return;
            }
            if (!IsSubscribedToLobbyUpdates()) {
                mLogger.addWarnNetLog("EWS::UnsubscribeToLobbyUpdates", "Not subscribed already");
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

        bool BeginCreateLobby() {
            // General sanity checks
            if (!IsInitialized()) {
                mLogger.addWarnNetLog("EWS::BeginCreateLobby", "Not initialized");
                return false;
            }
            if (!IsFullyLoggedIn()) {
                mLogger.addWarnNetLog("EWS::BeginCreateLobby", "Not yet logged in");
                return false;
            }
            if (mLobbyHandle == nullptr) {
                mLogger.addWarnNetLog("EWS::BeginCreateLobby", "mLobbyHandle is nullptr!");
                return false;
            }

            // Setup lobby options. Based on SDK sample's FLobbies::CreateLobby
            // FUTURE: Refactor relevant options for more flexible setup at EOS Wrapper level
            EOS_Lobby_CreateLobbyOptions createOptions = {};
            createOptions.ApiVersion = EOS_LOBBY_CREATELOBBY_API_LATEST;
            createOptions.LocalUserId = mLoggedInCrossPlatformId.getAccountId();
            createOptions.MaxLobbyMembers = 4;
            createOptions.PermissionLevel = EOS_ELobbyPermissionLevel::EOS_LPL_JOINVIAPRESENCE;
            createOptions.bPresenceEnabled = true; // Assuming no need for private presence
            createOptions.bAllowInvites = true;
            createOptions.BucketId = kDefaultGameLobbyBucketId;
            // Don't see any use case for turning this on atm (eg, instead have host via manager explicitly shut down lobby on leave)
            createOptions.bDisableHostMigration = EOS_FALSE;
            // Not using RTC (real-time chat) features atm
            createOptions.bEnableRTCRoom = EOS_FALSE;
            createOptions.LocalRTCOptions = nullptr;

            // Actually send the lobby creation request
            auto simpleCallback = [](const EOS_Lobby_CreateLobbyCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnCreateLobbyFinished(data);
            };
            EOS_Lobby_CreateLobby(mLobbyHandle, &createOptions, nullptr, simpleCallback);
            
            return true;
        }

        // Based on SDK samples' FLobbies::LeaveLobby
        bool BeginLeaveLobby(NetLobbySlot lobbySlot) {
            NetLobbyTracking& lobbyTrackingInfo = GetLobbyInfoForSlot(lobbySlot);
            
            // General sanity checks
            if (!IsInitialized()) {
                mLogger.addWarnNetLog("EWS::BeginCreateLobby", "Not initialized");
                return false;
            }
            if (!IsFullyLoggedIn()) {
                mLogger.addWarnNetLog("EWS::BeginCreateLobby", "Not yet logged in");
                return false;
            }
            if (mLobbyHandle == nullptr) {
                mLogger.addWarnNetLog("EWS::BeginCreateLobby", "mLobbyHandle is nullptr!");
                return false;
            }
            // Specific lobby sanity checks
            if (!lobbyTrackingInfo.IsActive()) {
                mLogger.addWarnNetLog("EWS::BeginCreateLobby", "Given lobby is not currently active");
                return false;
            }

            // Update state before actual leave lobby in case callback is immediately called for some reason
            mMainMatchLobby.SetLeaveInProgressState(mLogger);

            // TODO: If <= 1 members in lobby (and owner just in case), then destroy lobby instead. Also figure out why samples do this.

            EOS_Lobby_LeaveLobbyOptions leaveOptions = {};
            leaveOptions.ApiVersion = EOS_LOBBY_LEAVELOBBY_API_LATEST;
            leaveOptions.LobbyId = lobbyTrackingInfo.GetLobbyProperties().GetId().c_str();
            leaveOptions.LocalUserId = mLoggedInCrossPlatformId.getAccountId();

            // Do actual leave call
            auto simpleCallback = [](const EOS_Lobby_LeaveLobbyCallbackInfo* data) {
                Singleton<EOSWrapperSingleton>::get().OnLeaveLobbyFinished(data);
            };
            EOS_Lobby_LeaveLobby(mLobbyHandle, &leaveOptions, LobbySlotToPointerType(lobbySlot), simpleCallback);
            
            return true;
        }

        bool DestroyLobby(NetLobbyTracking& lobbyTracking) {
            // TODO:
            return false;
        }

        NetLobbyTracking& GetLobbyInfoForSlot(NetLobbySlot lobbySlot) {
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

#pragma region Status Helpers
        void SetNetStatus(EOSWrapperStatus newStatus) {
            mCurrentNetStatus = newStatus;

            if (mNetStatusChangeCallback) {
                mNetStatusChangeCallback(mCurrentNetStatus);
            }
        }

        bool IsInitialized() {
            return mCurrentNetStatus != EOSWrapperStatus::NotInitialized;
        }

        bool IsLoggedInAtAll() { // Not sure if "Basic" login even matters much, but staying on safe side with this method
            return IsBasicLoggedIn() || IsCrossPlatformLoggedIn();
        }
        bool IsFullyLoggedIn() { // Nice little wrapper to be explicit for what the cross platform login means
            return IsCrossPlatformLoggedIn();
        }
        
        bool IsBasicLoggedIn() {
            return mLoggedInEpicAccountId.isValid();
        }
        bool IsCrossPlatformLoggedIn() {
            return mLoggedInCrossPlatformId.isValid();
        }
#pragma endregion

#pragma region EOS SDK type helpers
        std::string ResultCodeToString(const EOS_EResult& result) {
            return EOS_EResult_ToString(result);
        }

        EOS_EPacketReliability ConvertPacketReliability(PacketReliability inPacketReliability) {
            switch (inPacketReliability) {
                case PacketReliability::UnreliableUnordered:
                    return EOS_EPacketReliability::EOS_PR_UnreliableUnordered;
                case PacketReliability::ReliableUnordered:
                    return EOS_EPacketReliability::EOS_PR_ReliableUnordered;
                case PacketReliability::ReliableOrdered:
                    return EOS_EPacketReliability::EOS_PR_ReliableOrdered;
                default:
                    mLogger.logWarnMessage(
                        "EWS::convertPacketReliability",
                        "Unexpected PacketReliability value, perhaps missing case?"
                    );
                return EOS_EPacketReliability::EOS_PR_ReliableOrdered;
            }
        }
#pragma endregion

        const std::string kTestSocketName = "TEST_SOCKET";
        // Below comment form SessionMatchmaking.cpp in SDK Samples:
        //      The top - level, game - specific filtering information for session searches. This criteria should be set with mostly static, coarse settings, often formatted like "GameMode:Region:MapName".
        static constexpr char kDefaultGameLobbyBucketId[] = "TODO:GameMode:Region";
        
        LoggerSingleton& mLogger = Singleton<LoggerSingleton>::get();
        EOSWrapperStatus mCurrentNetStatus = EOSWrapperStatus::NotInitialized;

        NetStatusChangeCallback mNetStatusChangeCallback = nullptr;
        NetGotSelfIdCallback mNetGotSelfIdCallback = nullptr;
        IEOSWrapperManager* mEosWrapperManager = nullptr;

        // Current user state
        EpicAccountIdWrapper mLoggedInEpicAccountId = {};
        CrossPlatformIdWrapper mLoggedInCrossPlatformId = {};
        NetLobbyTracking mMainMatchLobby = {};

        // Various EOS handles
        EOS_HPlatform mPlatformHandle = nullptr;
        // Note that the below handles depend on logged in state given current implementation
        //      ie, haven't yet tested if can retrieve these before logging in.
        EOS_HAuth mAuthHandle = nullptr;
        EOS_HConnect mConnectHandle = nullptr;
        EOS_HUI mUIHandle = nullptr;
        EOS_HLobby mLobbyHandle = nullptr;

        // Id used with managing incoming connection notifications. Not the same as a socket id or such
        EOS_NotificationId mConnectionNotificationId = EOS_INVALID_NOTIFICATIONID;
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
