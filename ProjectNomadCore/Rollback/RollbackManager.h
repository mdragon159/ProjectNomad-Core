#pragma once
#include "RollbackStaticSettings.h"
#include "GameCore/PlayerId.h"
#include "GameCore/PlayerInput.h"
#include "Utilities/FrameType.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    class RollbackManager {
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
        
        // Buffer to retrieve inputs. Head represents current frame + input delay
        //      Size is MaxRollblackFrames + MaxInputDelay to cover worst edge case for data storage
        //      (ie, if have max input delay AND need to rollback to the furthest back possible frame)
        using InputsBuffer = RingBuffer<PlayerInput, RollbackStaticSettings::MaxRollbackFrames + RollbackStaticSettings::MaxInputDelay>;
        InputsBuffer inputBufferForPlayer1;
        InputsBuffer inputBufferForPlayer2;

        // Settings
        FrameType currentInputDelay = RollbackStaticSettings::OnlineInputDelay; // NOTE: Assuming both players have same input delay
        bool isLocalPlayer1 = true; // For now hardcoding only two players
        
        // Various normal processing state
        bool isInitialized = false;
        FrameType latestLocalFrame = 0;
    
    public:
        RollbackManager() {
            resetState();
        }

        // Expected to call this method before normal usage. Furthermore, call this method if game ends and new game is started
        void initializeForGameStart(PlayerId localPlayerId) {
            resetState();
            isLocalPlayer1 = localPlayerId.playerSpot == PlayerSpot::Player1;

            isInitialized = true;
        }
        
        /// <summary>
        /// This function runs the core rollback logic.
        /// This should be called before any relevant gameplay logic and once per frame.
        /// ie, if rolling back, then *do not* call this for every re-calculated frame.
        /// </summary>
        /// <param name="currentFrame">Should always be one more than the previously called frame</param>
        /// <param name="localPlayerInput">Current frame's input for the local player</param>
        void doFrameUpdate(FrameType currentFrame, const PlayerInput& localPlayerInput) {
            if (!isInitialized) {
                logger.logWarnMessage("RollbackManager::doFrameUpdate", "Not initialized!");
                return;
            }
            if (latestLocalFrame != 0 && currentFrame != latestLocalFrame + 1) { // If not first frame and not strictly calling +1 frame...
                logger.logWarnMessage(
                    "RollbackManager::doFrameUpdate",
                    "Unexpected currentFrame value! latestLocalFrame: " + std::to_string(latestLocalFrame)
                        + ", currentFrame: " + std::to_string(currentFrame)
                );
            }

            getLocalPlayerInputBuffer().add(localPlayerInput);
            latestLocalFrame = currentFrame;
        }

        // TODO: Snapshot stuff
        void onGameFrameEnd() {}

        /// <summary>Retrieves input for a given frame. Assumes doFrameUpdate() has been called for every frame already</summary>
        /// <param name="playerId">Player to retrieve inputs for</param>
        /// <param name="frameToRetrieveInputsFor">
        /// Frame to retrieve inputs for. Must be <= current frame and within max rollback range
        /// </param>
        /// <returns>Intended player input for given player on given frame</returns>
        PlayerInput getPlayerInput(PlayerId playerId, FrameType frameToRetrieveInputsFor) {
            if (frameToRetrieveInputsFor > latestLocalFrame) {
                logger.logWarnMessage(
                    "RollbackManager::getInput",
                    "Provided retrieval frame greater than latest frame"
                );
                return {};
            }

            FrameType frameOffset = latestLocalFrame - frameToRetrieveInputsFor;
            if (frameOffset > RollbackStaticSettings::MaxRollbackFrames) {
                logger.logWarnMessage(
                    "RollbackManager::getInput",
                    "Provided retrieval frame greater than max rollback range"
                );
                return {};
            }

            frameOffset += currentInputDelay; // Simple way to apply arbitrary input delay

            switch(playerId.playerSpot) {
                case PlayerSpot::Player1:
                    return inputBufferForPlayer1.get(frameOffset);
                case PlayerSpot::Player2:
                    return inputBufferForPlayer2.get(frameOffset);

                default:
                    logger.logWarnMessage(
                    "RollbackManager::getInput",
                    "Unexpected player id spot. Likely switch statement case miss"
                    );
                    return {};
            }
        }

    private:
        void resetState() {
            isInitialized = false; // Just in case but shouldn't be necessary
            latestLocalFrame = 0;
            
            // Prefill input buffer with input delay's worth of data
            // This will guarantee input buffer has enough data to "look back" at the beginning of the game
            //      FUTURE: Note that the for loop may not be necessary depending on what `= {}` does exactly
            inputBufferForPlayer1 = {};
            inputBufferForPlayer2 = {};
            for (FrameType i = 0; i < currentInputDelay; i++) {
                inputBufferForPlayer1.add({});
                inputBufferForPlayer2.add({});
            }
        }

        InputsBuffer& getLocalPlayerInputBuffer() {
            if (isLocalPlayer1) {
                return inputBufferForPlayer1;
            }
            else {
                return inputBufferForPlayer2;
            }
        }
    };
}
