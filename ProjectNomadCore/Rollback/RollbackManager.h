#pragma once
#include "InputBuffer.h"
#include "RollbackCommunicationHandler.h"
#include "RollbackStaticSettings.h"
#include "RollbackUpdateResult.h"
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
        FrameType latestLocalStoredInputFrame = 0;
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
        /// This function runs the core rollback logic and intended to run before any gameplay logic occurs.
        /// If ProceedNormally or Rollback decision is returned, then this method should not be called for the same frame again.
        /// ie, if rolling back, then *do not* call this for every re-calculated frame.
        /// </summary>
        /// <param name="currentFrame">Should always be one more than the previously called frame</param>
        /// <param name="localPlayerInput">Current frame's input for the local player</param>
        /// <returns>Returns whether game should proceed normally, pause for a frame, or roll back</returns>
        RollbackUpdateResult doFrameUpdate(FrameType currentFrame, const PlayerInput& localPlayerInput) {
            if (!isInitialized) {
                logger.logWarnMessage("RollbackManager::doFrameUpdate", "Not initialized!");
                return { RollbackDecision::ProceedNormally };
            }

            // Only handle the input for a given frame once
            // Eg, even if we're skipping this frame, then store the input and communicate it to MP session (if any)
            if (currentFrame == latestLocalStoredInputFrame + 1) {
                handleLocalFrameInput(currentFrame, localPlayerInput);
            }

            // If lockstep setting on, then wait for input from other player before proceeding
            // This acts as a quick lockstep implementation test as well as a good way to test waiting logic
            if (RollbackStaticSettings::UseLockstep && isMultiplayerGame && latestRemotePlayerFrame < currentFrame) {
                // logger.logInfoMessage(
                //     "RollbackManager::doFrameUpdate",
                //     "Skipping frame to be in lockstep. Current frame: " + std::to_string(currentFrame)
                //         + ", Remote frame: " + std::to_string(latestRemotePlayerFrame)
                // );
                return { RollbackDecision::WaitFrame };
            }

            // Sanity check: If this isn't strictly one frame more than the previous frame, then something went wrong
            if (latestLocalFrame != 0 && currentFrame != latestLocalFrame + 1) {
                logger.logWarnMessage(
                    "RollbackManager::doFrameUpdate",
                    "Unexpected currentFrame value! latestLocalFrame: " + std::to_string(latestLocalFrame)
                        + ", currentFrame: " + std::to_string(currentFrame)
                );
            }

            // Finally remember that this frame was processed and allow game to proceed normally
            latestLocalFrame = currentFrame;
            return { RollbackDecision::ProceedNormally };
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
            // FUTURE: Should be safer way to handle these casts.
            //          Size checks *shouldn't* be necessary from a strictly security standpoint due to no pointers (simple data),
            //            BUT rather not leave it as an opening that could be potentially combined with a different opening
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

        void handleLocalFrameInput(FrameType currentFrame, const PlayerInput& localPlayerInput) {
            if (currentFrame != latestLocalStoredInputFrame + 1) { // Sanity check. Likely extraneous but nice to have
                logger.logWarnMessage(
                    "RollbackManager::handleLocalFrameInput",
                    "Unexpected current frame! Current frame: " + std::to_string(currentFrame)
                        + ", latestLocalStoredInputFrame: " + std::to_string(latestLocalStoredInputFrame)
                );
                return; // Don't continue as storing additional data will break input buffer frame tracking assumptions
            }
            
            // Store player input to be retrieved on the appropriate frame (which may be a latter frame due to input lag)
            InputBuffer& localInputsBuffer = getLocalPlayerInputBuffer();
            localInputsBuffer.add(localPlayerInput);

            if (isMultiplayerGame) {
                communicationHandler.sendInputsToRemotePlayer(currentFrame, localInputsBuffer);
            }
            
            latestLocalStoredInputFrame = currentFrame;
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
            // logger.logInfoMessage(
            //     "RollbackManager::handleInputUpdateFromConnectedPlayer",
            //     "Successfully processed InputsUpdateMessage for frame " + std::to_string(inputUpdateMessage.updateFrame)
            //     + " with latest jump input: " + std::to_string(inputUpdateMessage.playerInputs[0].isJumpPressed)
            // );
        }
    };
}
