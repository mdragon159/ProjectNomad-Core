#pragma once

#include "Rollback/Model/RollbackPerPlayerInputs.h"
#include "Rollback/Model/RollbackSettings.h"
#include "Input/CharacterInput.h"
#include "Input/PlayerInputsForFrame.h"
#include "Utilities/LoggerSingleton.h"

namespace ProjectNomad {
    /**
    * Manages input storage and retrieval (including for predictions) regarding all players
    **/
    class RollbackInputManager {
      public:
        bool SetupForNewSession(LoggerSingleton& logger,
                                const RollbackSettings& rollbackSettings) {
            mIsInitialized = false; // Set at start in case of failure
            
            // Validation check
            if (rollbackSettings.totalPlayers < 1 || rollbackSettings.totalPlayers > PlayerSpotHelpers::kMaxPlayerSpots) {
                logger.LogInfoMessage("Invalid total players setting: " + std::to_string(rollbackSettings.totalPlayers));
                return false;
            }
            
            mTotalPlayersInSession = rollbackSettings.totalPlayers;

            for (int i = 0; i < mTotalPlayersInSession; i++) {
                RollbackPerPlayerInputs& perPlayerInputs = mPerPlayerInputs[i]; // For readability
                
                // Re-init to assure no carryover of data from prior sessions
                perPlayerInputs = {};
                // Do any necessary setup for this session
                bool didSetupSucceed = perPlayerInputs.SetupForNewSession(logger, rollbackSettings);

                if (!didSetupSucceed) {
                    logger.LogInfoMessage("An inner setup call failed!");
                    return false;
                }
            }

            mIsInitialized = true;
            return true;
        }
        
        void SetInputForPlayer(LoggerSingleton& logger,
                               FrameType targetFrame,
                               PlayerSpot playerSpot,
                               const CharacterInput& playerInput) {
            if (!mIsInitialized) {
                logger.LogWarnMessage("Not initialized!");
                return;
            }

            uint32_t index = PlayerSpotToIndex(logger, playerSpot);
            mPerPlayerInputs[index].AddInput(logger, targetFrame, playerInput);
        }

        // Intended to be used for comparing if prior prediction incorrect.
        // However, almost certainly going to need to expand on this to properly cover different cases.
        // Eg, how does consumer know whether an input was a "predicted" or "confirmed" input with these APIs atm?
        const CharacterInput& GetPlayerInputForFrame(LoggerSingleton& logger,
                                                     FrameType targetFrame,
                                                     PlayerSpot playerSpot) const {
            if (!mIsInitialized) {
                logger.LogWarnMessage("Not initialized!");
                return temp;
            }

            uint32_t index = PlayerSpotToIndex(logger, playerSpot);
            return mPerPlayerInputs[index].GetInputForFrame(logger, targetFrame);
        }

        // Get set of inputs needed to process the given frame. Will try to predict inputs for any that are missing
        PlayerInputsForFrame GetInputsForFrame(LoggerSingleton& logger, FrameType targetFrame) const {
            if (!mIsInitialized) {
                logger.LogWarnMessage("Not initialized!");
                return {};
            }
            
            PlayerInputsForFrame result = {};
            for (int i = 0; i < mTotalPlayersInSession; i++) {
                const CharacterInput& playerInput = GetPlayerInputForFrame(logger, targetFrame, static_cast<PlayerSpot>(i));
                result.Add(playerInput);
            }
            
            return result;
        }

        FrameType GetLastStoredFrameForPlayer(LoggerSingleton& logger, PlayerSpot playerSpot) const {
            if (!mIsInitialized) {
                logger.LogWarnMessage("Not initialized!");
                return 0;
            }

            uint32_t index = PlayerSpotToIndex(logger, playerSpot);
            return mPerPlayerInputs[index].GetLastStoredFrame();
        }

        // Useful to confirm if missing too many inputs to process the next frame and thus should gameplay "delay" (freeze)
        bool IsFrameOutsideOfGetRangeForAnyPlayer(LoggerSingleton& logger,
                                                  FrameType targetFrame,
                                                  std::vector<PlayerSpot>& resultWaitingOnPlayers) const {
            if (!mIsInitialized) {
                logger.LogWarnMessage("Not initialized!");
                return false;
            }
            
            bool isAnyPlayerMissingTooManyInputs = false;
            for (int i = 0; i < mTotalPlayersInSession; i++) {
                const RollbackPerPlayerInputs& perPlayerInputs = mPerPlayerInputs[i]; // For readability
                
                if (perPlayerInputs.IsFrameOutsideOfGetRange(targetFrame)) {
                    isAnyPlayerMissingTooManyInputs = true;
                    resultWaitingOnPlayers.push_back(static_cast<PlayerSpot>(i)); // For feedback on who waiting on exactly
                }
            }

            return isAnyPlayerMissingTooManyInputs;
        }

        /**
        * Checks if any player in current match have not yet stored input for the given frame.
        * Intended to be used for quickly sanity checking that all expected inputs have been stored for a frame as it
        * exits the rollback window.
        *
        * FUTURE: This has overlap with IsFrameOutsideOfGetRangeForAnyPlayer(), albeit used in slightly different
        *           contexts (where former is used before processing new frame, and this is used afterwards).
        *           Would be nice to not have two such similar use case methods.
        * @param logger - Logger reference
        * @param targetFrame - Frame to check if input was ever stored for. Note that this may be outside of what inputs
        *                      are actually stored (ie, beyond the rollback window).
        * @returns true if any player has never gotten actual input for the given frame, false otherwise
        **/
        bool DoesAnyPlayerNotYetHaveInputForFrame(LoggerSingleton& logger, FrameType targetFrame) const {
            if (!mIsInitialized) {
                logger.LogWarnMessage("Not initialized!");
                return false;
            }

            // Check if target frame hasn't yet been stored for any player
            for (int i = 0; i < mTotalPlayersInSession; i++) {
                const RollbackPerPlayerInputs& perPlayerInputs = mPerPlayerInputs[i]; // For readability

                // If target frame is beyond the latest stored frame, then this player hasn't yet stored input for the given frame.
                //      This relies on fact that inputs are always stored incrementally. Ie, if frame 20 is stored,
                //      then frames 0-19 have already been stored at some point in the past
                if (targetFrame > perPlayerInputs.GetLastStoredFrame()) {
                    return true;
                }
            }
            
            return false; // Confirmed that all players have stored input for the target frame at some point
        }

      private:
        uint32_t PlayerSpotToIndex(LoggerSingleton& logger, PlayerSpot playerSpot) const {
            auto result = static_cast<uint32_t>(playerSpot); // Enum starts at 0

            // Sanity check: If index goes representing total # of players
            if (result + 1 > mTotalPlayersInSession) {
                logger.LogWarnMessage(
                    "Spot is out of range. Total players in session: " + std::to_string(mTotalPlayersInSession) +
                    ", input player spot: " + std::to_string(result)
                );
                return 0; // Fallback value just in case
            }

            return result;
        } 
        
        CharacterInput temp = {};

        bool mIsInitialized = false;
        uint8_t mTotalPlayersInSession = 1; // Should always be greater than 0 in an actual game
        // Actually store input on a per-player basis.
        //      Using an array of per-player input "sub-managers" like this to try to reduce complexity compared to
        //      managing all players' inputs at once.
        RollbackPerPlayerInputs mPerPlayerInputs[PlayerSpotHelpers::kMaxPlayerSpots] = {};
    };
}
