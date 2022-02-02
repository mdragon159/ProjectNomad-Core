#pragma once
#include "InputBuffer.h"
#include "RollbackCommunicationHandler.h"
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
        RollbackCommunicationHandler communicationHandler;
        
        InputBuffer inputBufferForPlayer1;
        InputBuffer inputBufferForPlayer2;

        // Settings
        FrameType currentInputDelay = RollbackStaticSettings::OnlineInputDelay; // NOTE: Assuming both players have same input delay
        bool isLocalPlayer1 = true; // For now hardcoding only two players
        bool isMultiplayerGame = false;
        
        // Various normal processing state
        bool isInitialized = false;
        FrameType latestLocalFrame = 0;
        FrameType latestRemotePlayerFrame = 0;
    
    public:
        RollbackManager() {
            resetState();
        }

        // Expected to call this method before normal usage. Furthermore, call this method if game ends and new game is started
        void initializeForNewGame(PlayerId localPlayerId) {
            resetState();
            isLocalPlayer1 = localPlayerId.playerSpot == PlayerSpot::Player1;
            isMultiplayerGame = communicationHandler.isMultiplayerGame();

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
            // TODO: If lockstep setting on, then wait for input from other player before proceeding
            
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

            InputBuffer& localInputsBuffer = getLocalPlayerInputBuffer();
            localInputsBuffer.add(localPlayerInput);

            if (isMultiplayerGame) {
                communicationHandler.sendInputsToRemotePlayer(currentFrame, localInputsBuffer);
            }

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

        void onMessageReceivedFromConnectedPlayer(NetMessageType messageType, const std::vector<char>& messageData) {
            if (!isMultiplayerGame) {
                logger.logWarnMessage(
                "RollbackManager::onMessageReceivedFromConnectedPlayer",
                "Manager not initialized to be a multiplayer game! \
                                Current expectation is that MP connection is setup BEFORE RollbackManager is initialized"
                );
                return;
            }
            
            // FUTURE: Would be nice if code is refactored such that this handling can happen in the CommunicationHandler
            // FUTURE: Should be safer way to handle these casts. Perhaps at least do size checks?
            switch (messageType) {
                case NetMessageType::InputUpdate: {
                    const InputUpdateMessage* updateMessage = reinterpret_cast<const InputUpdateMessage*>(messageData.data());
                    handleInputUpdateFromConnectedPlayer(*updateMessage);
                    break;
                }

                default:
                    logger.logWarnMessage(
                        "RollbackManager::onMessageReceivedFromConnectedPlayer",
                        "Message type not in list of handled cases: " + std::to_string(static_cast<int>(messageType))
                );
            }
        }

    private:
        void resetState() {
            isInitialized = false; // Just in case but shouldn't be necessary
            latestLocalFrame = 0;
            latestRemotePlayerFrame = 0;
            
            // Prefill input buffer with input delay's worth of data
            // This will guarantee input buffer has enough data to "look back" at the beginning of the game
            //      FUTURE: Note that the for loop may not be necessary depending on what `= {}` does exactly
            //      FUTURE: Also prefilling with enough data for INPUTS_HISTORY_SIZE. Either should explicitly choose max value
            //              or pick a better approach
            inputBufferForPlayer1 = {};
            inputBufferForPlayer2 = {};
            for (FrameType i = 0; i < RollbackStaticSettings::MaxRollbackFrames; i++) {
                inputBufferForPlayer1.add({});
                inputBufferForPlayer2.add({});
            }
        }

        InputBuffer& getLocalPlayerInputBuffer() {
            if (isLocalPlayer1) {
                return inputBufferForPlayer1;
            }
            else {
                return inputBufferForPlayer2;
            }
        }

        InputBuffer& getRemotePlayerInputBuffer() {
            if (isLocalPlayer1) {
                return inputBufferForPlayer2;
            }
            else {
                return inputBufferForPlayer1;
            }
        }

        void handleInputUpdateFromConnectedPlayer(const InputUpdateMessage& inputUpdateMessage) {
            if (inputUpdateMessage.updateFrame <= latestRemotePlayerFrame) {
                logger.logWarnMessage(
                    "RollbackManager::handleInputUpdateFromConnectedPlayer",
                    "Received older frame input: " + std::to_string(inputUpdateMessage.updateFrame)
                );
                return;
            }

            FrameType amountOfMissingInputs = inputUpdateMessage.updateFrame - latestRemotePlayerFrame;
            if (amountOfMissingInputs > RollbackStaticSettings::MaxRollbackFrames) {
                logger.logWarnMessage(
                    "RollbackManager::handleInputUpdateFromConnectedPlayer",
                    "Received input for frame outside rollback window. Frame: " + std::to_string(inputUpdateMessage.updateFrame)
                );
                // TODO: End game or otherwise resolve the inconsistency!
                return;
            }

            // Add missing data
            // FUTURE: Have two separate variables here, INPUTS_HISTORY_SIZE and MaxRollbackFrames. Right now they're equal,
            //          but in future they could be different. At risk here to have a bug when those don't equal
            InputBuffer& remoteInputsBuffer = getRemotePlayerInputBuffer();
            for (FrameType i = 0; i < amountOfMissingInputs; i++) {
                // index 0 is the latest frame so add backwards
                const PlayerInput& remotePlayerInput = inputUpdateMessage.playerInputs.at(amountOfMissingInputs - i);
                remoteInputsBuffer.add(remotePlayerInput);
            }

            // TODO: Check against prediction and remember to roll back if necessary
            
            latestRemotePlayerFrame = inputUpdateMessage.updateFrame;

            // Some temp debug/verification logging
            logger.logInfoMessage(
                "RollbackManager::handleInputUpdateFromConnectedPlayer",
                "Successfully processed InputsUpdateMessage for frame " + std::to_string(inputUpdateMessage.updateFrame)
                + " with latest jump input: " + std::to_string(inputUpdateMessage.playerInputs[0].isJumpPressed)
            );
        }
    };
}
