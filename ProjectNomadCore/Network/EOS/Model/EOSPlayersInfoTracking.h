#pragma once

#include <set>

#include "CrossPlatformIdWrapper.h"
#include "EpicAccountIdWrapper.h"
#include "EOSHashFunction.h"
#include "EOSPlayerInfo.h"
#include "EOS/Include/eos_userinfo.h"
#include "Utilities/LoggerSingleton.h"

namespace ProjectNomad {
    /**
    * Encapsulates tracking and storing EOS-related general info regarding players interacting with, such as display names.
    * 
    * Note that most of the complexity comes from how many steps is required to actually pull the general info from the
    * EOS SDK.
    **/
    class EOSPlayersInfoTracking {
      public:
        /* Avoid access violation crash via copying each "list" in the return rather than returning by reference.
         * The crash occurs due to callers using a foreach loop while calling our non-const methods, thus modifyying
         * the data structure they were looping over.
         *
         * We *can* shift the responsibility onto callers here, but rather be safe and wait to do that if/when
         * the performance savings would be clearly worth.
         * 
         * Crash logs below:
         * Unhandled Exception: EXCEPTION_ACCESS_VIOLATION reading address 0xffffffffffffffff
         * UnrealEditor_TheNomadGame!std::_Tree_unchecked_const_iterator<std::_Tree_val<std::_Tree_simple_types<ProjectNomad::CrossPlatformIdWrapper> >,std::_Iterator_base0>::operator++() [C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133\INCLUDE\xtree:51
         * (Next line was on the actual foreach loop itself)
         */
        std::set<CrossPlatformIdWrapper> GetNotYetSentAccountMappingQueries() const {
            return mNotSentYetAccountMappingQueries;
        }
        std::set<EpicAccountIdWrapper> GetNotYetSentUserInfoQueries() const {
            return mNotSentYetUserInfoQueries;
        }
        std::set<CrossPlatformIdWrapper> GetCurrentlyQueriedAccountMappings() const {
            return mCurrentlyQueriedAccountMappings;
        }
        
        bool HasAnyNotSentQueries() const {
            return !mNotSentYetAccountMappingQueries.empty() || !mNotSentYetUserInfoQueries.empty();
        }
        bool HasAnyPendingRequests(LoggerSingleton& logger) const {
            bool hasAnyPendingRequests = !mPendingRequestedUserInfoWithEpicIds.empty()
                || !mPendingRequestedUserInfoWithoutEpicIds.empty();

            // Sanity checks to help identify any bugs
            if (!hasAnyPendingRequests) {
                if (HasAnyNotSentQueries()) {
                    logger.AddWarnNetLog(
                        "NetUsersInfoTracking::HasAnyPendingRequests",
                        "Have no pending requests but somehow still have not yet sent requests!"
                    );
                }
                if (!mCurrentlyQueriedAccountMappings.empty()) {
                    logger.AddWarnNetLog(
                        "NetUsersInfoTracking::HasAnyPendingRequests",
                        "Have no pending requests but currently queried mappings list is not empty!"
                    );
                }
            }
            
            return hasAnyPendingRequests;
        }

        bool CheckIfHaveUserInfoAndQueryIfNot(LoggerSingleton& logger, const CrossPlatformIdWrapper& targetId) {
            // Check if have EpicAccount id mapping yet (which is required for actual user info query)
            if (!mCrossPlatformToEpicAccountsMap.contains(targetId)) {
                // Begin querying for user info by first prepping query for the necessary account id
                StartAccountIdLookupQueryForUserInfoIfNecessary(logger, targetId);
                return false;
            }
            
            // Check if already have the desired info
            const EpicAccountIdWrapper& targetEpicAccountId = mCrossPlatformToEpicAccountsMap.at(targetId);
            if (!mCachedUserInfoMap.contains(targetEpicAccountId)) {
                StartUserInfoQueryIfNecessary(logger, targetEpicAccountId, targetId);
                return false;
            }
            
            // Otherwise we do have the user info already!
            return true;
        }

        const EOSPlayerInfo& GetCachedPlayerInfoIfAny(LoggerSingleton& logger, const CrossPlatformIdWrapper& targetId) const {
            // Need to get epic id as user info is queried by the epic id
            if (!mCrossPlatformToEpicAccountsMap.contains(targetId)) {
                return kEmptyUserInfo;
            }

            // Try to get the actual user info
            const EpicAccountIdWrapper& targetEpicAccountId = mCrossPlatformToEpicAccountsMap.at(targetId);
            if (!mCachedUserInfoMap.contains(targetEpicAccountId)) {
                return kEmptyUserInfo;
            }

            // Finally return the stored info!
            return mCachedUserInfoMap.at(targetEpicAccountId);
        }

        void SetAccountIdMapping(LoggerSingleton& logger,
                                 const EpicAccountIdWrapper& epicAccountId,
                                 const CrossPlatformIdWrapper& crossPlatformId) {
            mCrossPlatformToEpicAccountsMap.insert({crossPlatformId, epicAccountId});
            mEpicAccountsToCrossPlatformMap.insert({epicAccountId, crossPlatformId});

            // If pending request for user info, then move forward to next bucket
            if (mPendingRequestedUserInfoWithoutEpicIds.contains(crossPlatformId)) {
                mPendingRequestedUserInfoWithoutEpicIds.erase(crossPlatformId);
                mCurrentlyQueriedAccountMappings.erase(crossPlatformId);
                
                StartUserInfoQueryIfNecessary(logger, epicAccountId, crossPlatformId);
            }
        }
        void SetQueriedUserInfo(LoggerSingleton& logger,
                                const EpicAccountIdWrapper& targetId,
                                const EOS_UserInfo& eosUserInfo) {
            // Convert EOS struct to internal representation
            EOSPlayerInfo resultInfo = {};
            resultInfo.hasInfo = true;
            resultInfo.sanitizedDisplayName = eosUserInfo.DisplayNameSanitized;

            // Add to internal tracking for later lookup
            mCachedUserInfoMap.insert({targetId, resultInfo});
            
            // Clear any pending requests for this info
            if (mPendingRequestedUserInfoWithEpicIds.contains(targetId)) { // Checking first may not be strictly necessary but I like this readability
                logger.AddInfoNetLog(
                    "NetUsersInfoTracking::SetQueriedUserInfo",
                    "Successfully retrieved info for pending request with display name: " + resultInfo.sanitizedDisplayName
                );
                mPendingRequestedUserInfoWithEpicIds.erase(targetId);
            }
        }

        void MarkAllNotYetSentQueriesAsSent() {
            // Add all of now-sent account mapping queries to active queried list as need to iterate through em (see usage)
            for (const CrossPlatformIdWrapper& nowSentId : mNotSentYetAccountMappingQueries) {
                if (!mCurrentlyQueriedAccountMappings.contains(nowSentId)) {
                    mCurrentlyQueriedAccountMappings.insert(nowSentId);
                }
            }

            // Clear the actual not-yet-sent lists as actually sent now
            mNotSentYetAccountMappingQueries = {};
            mNotSentYetUserInfoQueries = {};
        }

        // Support clearing pending requests even if failure occurs so updates aren't blocked on these failures.
        //      ie, at time of writing we want a callback to netcode "user" side once all pending requests are done.
        //      Thus, we should clear out the failures so that callback can still happen one way or another.
        void ClearAllPendingAccountIdRequestsDueToFailure() {
            mPendingRequestedUserInfoWithoutEpicIds.clear();
            mCurrentlyQueriedAccountMappings.clear();
        }
        void ClearPendingAccountIdRequestDueToFailure(const CrossPlatformIdWrapper& crossPlatformId) {
            mPendingRequestedUserInfoWithoutEpicIds.erase(crossPlatformId);
            mCurrentlyQueriedAccountMappings.erase(crossPlatformId);
        }
        void ClearPendingUserInfoRequestDueToFailure(const EpicAccountIdWrapper& epicAccountId) {
            mPendingRequestedUserInfoWithEpicIds.erase(epicAccountId);
        }

      private:
        void StartAccountIdLookupQueryForUserInfoIfNecessary(LoggerSingleton& logger, const CrossPlatformIdWrapper& targetId) {
            // If request already exists, then do nothing to prevent unnecessary duplicate queries
            if (mPendingRequestedUserInfoWithoutEpicIds.contains(targetId)) {
                logger.AddInfoNetLog(
                    "NetUsersInfoTracking::StartAccountIdLookupQueryForUserInfoIfNecessary",
                        "Already have pending request for: " + targetId.ToStringForLogging()
                );
                return;
            }

            logger.AddInfoNetLog(
                "NetUsersInfoTracking::StartAccountIdLookupQueryForUserInfoIfNecessary",
                "Storing account mapping request for cross platform id: " + targetId.ToStringForLogging()
            );
            
            // Setup the actual request.
            //      Not immediately sending out the request as letting EOSWrapperSingleton handle that, as simple chosen
            //       workaround for circular dependencies x EOS callbacks requiring singleton/static context
            mPendingRequestedUserInfoWithoutEpicIds.insert(targetId);
            mNotSentYetAccountMappingQueries.insert(targetId);
        }

        void StartUserInfoQueryIfNecessary(LoggerSingleton& logger,
                                           const EpicAccountIdWrapper& epicAccountId,
                                           const CrossPlatformIdWrapper& crossPlatformId) {
            // If request already exists, then do nothing to prevent unnecessary duplicate queries
            if (mPendingRequestedUserInfoWithEpicIds.contains(epicAccountId)) {
                logger.AddInfoNetLog(
                    "NetUsersInfoTracking::StartUserInfoQueryIfNecessary",
                    "Already have pending request for: " + epicAccountId.ToStringForLogging() +
                    " and cross platform id: " + crossPlatformId.ToStringForLogging()
                );
                return;
            }

            logger.AddInfoNetLog(
                "NetUsersInfoTracking::StartUserInfoQueryIfNecessary",
                "Storing user info request for epic account id: " + epicAccountId.ToStringForLogging() +
                " and cross platform id: " + crossPlatformId.ToStringForLogging()
            );

            // Setup the actual request.
            mPendingRequestedUserInfoWithEpicIds.insert(epicAccountId);
            mNotSentYetUserInfoQueries.insert(epicAccountId);
        }

        static inline const EOSPlayerInfo kEmptyUserInfo = {}; // Simple workaround for passing default value by reference

        // Mapping of EpicAccountId to CrossPlatformId (both ways), cuz dang it why do some APIs need either one?!
        //      Like Lobby APIs only give CrossPlatformId of other members, BUT QueryUserInfo only takes EpicAccountId
        std::unordered_map<CrossPlatformIdWrapper, EpicAccountIdWrapper, EOSHashFunction> mCrossPlatformToEpicAccountsMap = {};
        std::unordered_map<EpicAccountIdWrapper, CrossPlatformIdWrapper, EOSHashFunction> mEpicAccountsToCrossPlatformMap = {};
        // Mapping of final result for user info queries
        std::unordered_map<EpicAccountIdWrapper, EOSPlayerInfo, EOSHashFunction> mCachedUserInfoMap = {};

        // Tracking individual pending user info requests.
        //      Split into two sequential buckets: Those without epic ids (waiting for account id query) and those with
        //      epic ids (waiting for actual user info query)
        std::set<CrossPlatformIdWrapper> mPendingRequestedUserInfoWithoutEpicIds = {};
        std::set<EpicAccountIdWrapper> mPendingRequestedUserInfoWithEpicIds = {};
        // Tracking what queries have not actually been sent yet, split by the sequential buckets mentioned above
        std::set<CrossPlatformIdWrapper> mNotSentYetAccountMappingQueries = {};
        std::set<EpicAccountIdWrapper> mNotSentYetUserInfoQueries = {};
        // Also tracking the queries that HAVE been sent for account mapping, as the darn API doesn't tell you what you sent yet
        //      Could do some smart-ness with changing mNotSentYetAccountMappingQueries to a set, but not much of a difference atm.
        std::set<CrossPlatformIdWrapper> mCurrentlyQueriedAccountMappings = {};
    };
}
