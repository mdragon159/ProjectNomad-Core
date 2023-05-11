#pragma once

#include <vector>

#include "GameCore/PlayerSpot.h"
#include "Network/EOS/Model/CrossPlatformIdWrapper.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Containers/FlexArray.h"

namespace ProjectNomad {
    using PlayerIdPerSpot = FlexArray<CrossPlatformIdWrapper, PlayerSpotHelpers::kMaxPlayerSpots>;

    /**
    * Defines "player spot" mapping for all players in current lobby. Eg, who is Player 1, who is Player 2, and so on.
    *
    * This is necessary as each player may have a different order of player ids from EOS.
    * Thus, we need to handle shared player spot logic + data ourselves.
    **/
    class NetPlayerSpotMapping {
      public:
        bool IsMappingSet() const {
            return mIsMappingSet;
        }
        bool IsLocked() const {
            return mIsLocked;
        }

        void SetLock(bool isLocked) {
            mIsLocked = isLocked;
        }
        
        void SetMapping(LoggerSingleton& logger,
                        const CrossPlatformIdWrapper& localPlayerId,
                        const std::vector<CrossPlatformIdWrapper>& allPlayerIdsInOrder) {
            mIsMappingSet = false; // Reset validity in case there are any issues later on

            // Sanity checks
            if (mIsLocked) { // Expected to check this before trying to call this method
                logger.AddWarnNetLog("Called but currently locked!");
                return;
            }
            if (PlayerSpotHelpers::IsInvalidTotalPlayers(allPlayerIdsInOrder.size())) {
                logger.AddWarnNetLog("Invalid # of player ids given! Provided size: " + std::to_string(mTotalPlayers));
                return;
            }

            mTotalPlayers = static_cast<uint8_t>(allPlayerIdsInOrder.size()); // Use static cast to get rid of "loss of precision" warnings
            
            // Store all the player ids against their intended spot
            bool wasLocalPlayerSpotFound = false;
            mPlayerIdPerSpot = {}; // This is a sort of "array"/"list" so make sure to reset before trying to add to it
            for (uint8_t i = 0; i < mTotalPlayers; i++) {
                const CrossPlatformIdWrapper& curId = allPlayerIdsInOrder.at(i);
                
                // Set the ids in order which represents the intended player spot as well
                mPlayerIdPerSpot.Add(curId);

                if (localPlayerId == curId) {
                    if (wasLocalPlayerSpotFound) { // Extremely unlikely sanity check but eh, super easy and low cost to check
                        logger.AddWarnNetLog("Found local player id more than once!");
                        return;
                    }
                    
                    wasLocalPlayerSpotFound = true;
                    mLocalPlayerSpot = static_cast<PlayerSpot>(i); // Already validated range earlier in function
                }
            }

            // Sanity check: Assure local player was in list of players.
            //      Currently not worrying about spectator support, will leave that for way in future
            if (!wasLocalPlayerSpotFound) {
                logger.AddWarnNetLog("Local player id not found in list of player ids!");
                return;
            }
            
            mIsMappingSet = true; // Successfully set up mapping!
        }

        uint8_t GetTotalPlayers() const {
            return mTotalPlayers;
        }
        PlayerSpot GetLocalPlayerSpot() const {
            return mLocalPlayerSpot;
        }
        const PlayerIdPerSpot& GetUnderlyingMapping() const {
            return mPlayerIdPerSpot;
        }

        bool TryGetPlayerSpotForId(LoggerSingleton& logger,
                                   const CrossPlatformIdWrapper& targetId,
                                   PlayerSpot& result) const {
            if (!mIsMappingSet) {
                logger.AddWarnNetLog("Called while not setup!");
                return false;
            }

            // Do a straightforward linear search through all possible player spots
            //      Yes, an actual map is guaranteed O(1) but total players is a very very small number.
            //      Performance impact should be negligible but we can profile this later in future to be sure.
            for (uint8_t i = 0; i < mTotalPlayers; i++) {
                if (targetId == mPlayerIdPerSpot.Get(i)) {
                    result = static_cast<PlayerSpot>(i); // total player count should never be out of range of PlayerSpot enum
                    return true;
                }
            }
            
            return false; // Id not found in mapping
        }

        bool TryGetPlayerIdForSpot(LoggerSingleton& logger,
                                   PlayerSpot targetSpot,
                                   CrossPlatformIdWrapper& result) const {
            if (!mIsMappingSet) {
                logger.AddWarnNetLog("Called while not setup!");
                return false;
            }

            auto targetIndex = static_cast<uint32_t>(targetSpot);
            
            // Validate input isn't out of bounds
            if (targetIndex + 1 > mTotalPlayers) {
                logger.AddWarnNetLog(
                    "Tried to call with out of range spot! Total players: " + std::to_string(mTotalPlayers) +
                    ", provided value: " + std::to_string(targetIndex)
                );
                return false;
            }

            // Simply retrieve the id from the correct "slot" (as we have a storage "slot" for every possible player spot)
            result = mPlayerIdPerSpot.Get(targetIndex);
            return true;
        }
        
      private:
        bool mIsMappingSet = false;
        bool mIsLocked = false; // Represents whether or not this mapping is currently locked from any further changes. (Really should be at Lobby level but this works for now)
        uint8_t mTotalPlayers = 1; // Expected to be >= 1 and <= max possible players
        
        // Assuming this will be something commonly looked up. Thus explicitly find and store this
        PlayerSpot mLocalPlayerSpot = PlayerSpot::Player1;
        
        // Underlying Mapping: Define a player id for each potential player spot.
        //      This relies on assumption that earlier spots will be used before latter spots.
        //      Eg, if only two players in a match, then only first two "slots" of this data structure will be used.
        //
        //      Note: Could instead use a map from id to spot (and vice versa), but this is simple and easy to do atm.
        //      Can consider performance cost of actual map vs this approach later
        PlayerIdPerSpot mPlayerIdPerSpot = {};
    };
}
