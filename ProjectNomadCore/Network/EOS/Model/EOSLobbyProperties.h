#pragma once
#include <string>

#include "EOSLobbyDetailsKeeper.h"
#include "Context/CoreContext.h"
#include "EOS/Include/eos_lobby.h"
#include "EOS/Include/eos_lobby_types.h"

namespace ProjectNomad {
    class EOSLobbyProperties {
      public:
        bool IsLobbyValid() const {
            return mIsLobbyValid;
        }
        const std::string& GetId() const {
            return mLobbyId;
        }
        bool IsLocalPlayerLobbyOwner() const {
            return mIsLocalPlayerLobbyOwner;
        }
        const CrossPlatformIdWrapper& GetLobbyOwner() const {
            return mOwnerId;
        }
        uint32_t GetMaxMembers() const {
            return mLobbyMaxMembers;
        }
        const std::vector<CrossPlatformIdWrapper>& GetMembersList() const {
            return mCurLobbyMembers;
        }
        
        // Based on SDK sample's FLobby::InitFromLobbyHandle
        bool TryInit(LoggerSingleton& logger,
                     const CrossPlatformIdWrapper& localPlayerId,
                     EOS_HLobby lobbyHandle,
                     EOS_LobbyId inputLobbyId) {
            mIsLobbyValid = false; // Assure starting state is invalid (so save time writing this at each fail case)

            // Sanity checks
            if (lobbyHandle == nullptr) {
                logger.AddWarnNetLog("EOSLobbyProperties::InitFromLobbyHandle", "Lobby handle is nullptr!");
                return false;
            }
            if (inputLobbyId == nullptr) {
                logger.AddWarnNetLog(
                    "EOSLobbyProperties::InitFromLobbyHandle",
                    "Input id is nullptr!"
                );
                return false;
            }

            // Retrieve the "handle" so can later retrieve actual lobby info
            EOS_HLobbyDetails lobbyDetailsHandle = TryRetrieveLobbyDetailsHandle(
                logger, localPlayerId, lobbyHandle, inputLobbyId
            );
            if (!lobbyDetailsHandle) {
                logger.AddWarnNetLog("EOSLobbyProperties::InitFromLobbyHandle", "Failed to retrieve lobby details handle");
                return false;
            }

            // Retrieve general lobby info
            uint32_t availableSlotsFromSDK = 0;
            if (!TryGetGeneralLobbyDetails(logger, localPlayerId, lobbyDetailsHandle, availableSlotsFromSDK)) {
                logger.AddWarnNetLog("EOSLobbyProperties::InitFromLobbyHandle", "Failed to retrieve general lobby details");
                return false;
            }

            // Retrieve basic info regarding each lobby member
            GetMembersDataForLobby(lobbyDetailsHandle);

            // Sanity check: Validate # of available slots reported by EOS vs calculated by stored data
            uint32_t calculatedAvailableSlots = mLobbyMaxMembers - mCurLobbyMembers.size();
            if (availableSlotsFromSDK != calculatedAvailableSlots) {
                logger.AddWarnNetLog(
                    "EOSLobbyProperties::InitFromLobbyHandle",
                    "Stored lobby data doesn't match with available slots from SDK. Available slots from SDK: "
                    + std::to_string(availableSlotsFromSDK) + ", calculated slots: " + std::to_string(calculatedAvailableSlots)
                );
                return false; // Make extra clear there was an issue
            }

            // Finally store this id data that'll come in handy with future lobby usage.
            //      Why not earlier? Just cuz don't wanna set a shared ptr if gonna fail (but doesn't *really* matter atm)
            mLobbyDetailsHandleKeeper = MakeLobbyDetailsKeeper(lobbyDetailsHandle);
            mLobbyId = inputLobbyId;

            mIsLobbyValid = true;
            return true;
        }

      private:
        static EOS_HLobbyDetails TryRetrieveLobbyDetailsHandle(LoggerSingleton& logger,
                                                               const CrossPlatformIdWrapper& localPlayerId,
                                                               EOS_HLobby lobbyHandle,
                                                               EOS_LobbyId inputLobbyId) {
            // Prepare to retrieve the handle to *then* retrieve the lobby details
            EOS_Lobby_CopyLobbyDetailsHandleOptions copyHandleOptions = {};
            copyHandleOptions.ApiVersion = EOS_LOBBY_COPYLOBBYDETAILSHANDLE_API_LATEST;
            copyHandleOptions.LobbyId = inputLobbyId;
            copyHandleOptions.LocalUserId = localPlayerId.GetAccountId();
            
            // Actually retrieve the lobby details handle
            EOS_HLobbyDetails lobbyDetailsHandle = nullptr;
            EOS_EResult resultCode;
            resultCode = EOS_Lobby_CopyLobbyDetailsHandle(lobbyHandle, &copyHandleOptions, &lobbyDetailsHandle);
            if (resultCode != EOS_EResult::EOS_Success)
            {
                logger.AddWarnNetLog(
                    "EOSLobbyProperties::InitFromLobbyHandle",
                    "Retrieving lobby details handle failed with result code: " + EOSHelpers::ResultCodeToString(resultCode)
                );
                return nullptr;
            }

            return lobbyDetailsHandle;
        }
        
        bool TryGetGeneralLobbyDetails(LoggerSingleton& logger,
                                       const CrossPlatformIdWrapper& localPlayerId,
                                       EOS_HLobbyDetails lobbyDetailsHandle,
                                       uint32_t& resultAvailableSlotsFromSDK) {
            EOS_LobbyDetails_CopyInfoOptions copyInfoDetails;
            copyInfoDetails.ApiVersion = EOS_LOBBYDETAILS_COPYINFO_API_LATEST;

            // Actually retrieve the lobby info
            EOS_LobbyDetails_Info* lobbyInfo = nullptr;
            EOS_EResult resultCode = EOS_LobbyDetails_CopyInfo(lobbyDetailsHandle, &copyInfoDetails, &lobbyInfo);
            if (resultCode != EOS_EResult::EOS_Success || !lobbyInfo)
            {
                logger.AddWarnNetLog(
                    "EOSLobbyProperties::InitFromLobbyHandle",
                    "Copying lobby details failed with result code: " + EOSHelpers::ResultCodeToString(resultCode)
                );
                return false;
            }

            // Copy basic lobby info
            resultAvailableSlotsFromSDK = lobbyInfo->AvailableSlots;
            mLobbyMaxMembers = lobbyInfo->MaxMembers;
            // FUTURE: Copy lobby permission info that user would be interested in (eg, is lobby invite only)
            
            // Copy owner info for easy retrieval
            mOwnerId = CrossPlatformIdWrapper(lobbyInfo->LobbyOwnerUserId);
            mIsLocalPlayerLobbyOwner = mOwnerId.GetAccountId() == localPlayerId.GetAccountId();
            
            // Release the lobby info copy pointer now that done with it
            EOS_LobbyDetails_Info_Release(lobbyInfo);

            return true; // Success!
        }
        
        void GetMembersDataForLobby(EOS_HLobbyDetails lobbyDetailsHandle) {
            mCurLobbyMembers.clear();
            
            // Get member count so can then loop through each member one by one
            EOS_LobbyDetails_GetMemberCountOptions memberCountOptions = {};
            memberCountOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERCOUNT_API_LATEST;
            const uint32_t memberCount = EOS_LobbyDetails_GetMemberCount(lobbyDetailsHandle, &memberCountOptions);

            // Actually get all members of this lobby
            for (uint32_t memberIndex = 0; memberIndex < memberCount; ++memberIndex) {
                // Retrieve this member's id
                EOS_LobbyDetails_GetMemberByIndexOptions memberOptions = {};
                memberOptions.ApiVersion = EOS_LOBBYDETAILS_GETMEMBERBYINDEX_API_LATEST;
                memberOptions.MemberIndex = memberIndex;
                EOS_ProductUserId memberId = EOS_LobbyDetails_GetMemberByIndex(lobbyDetailsHandle, &memberOptions);

                mCurLobbyMembers.push_back(CrossPlatformIdWrapper(memberId));

                // FUTURE: Copy member attributes here.
                //         Currently using no member attributes (like SDK sample's "current skin") so nothing to do atm.
            }
        }
        
        bool mIsLobbyValid = false;
        std::string mLobbyId = ""; // FUTURE: See if can replace all usages of this with the lobby details handle
        EOSLobbyDetailsKeeper mLobbyDetailsHandleKeeper = nullptr;

        bool mIsLocalPlayerLobbyOwner = false;
        CrossPlatformIdWrapper mOwnerId = {};

        uint32_t mLobbyMaxMembers = 0;
        std::vector<CrossPlatformIdWrapper> mCurLobbyMembers = {};
    };
}
