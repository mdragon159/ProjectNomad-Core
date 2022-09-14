#pragma once

#include "InputBuffer.h"
#include "Rollback/InputPredictor.h"
#include "RollbackCommunicationHandler.h"
#include "OldRollbackManagerGameState.h"
#include "Rollback/RollbackSnapshotManager.h"
#include "OldRollbackStaticSettings.h"
#include "RollbackUpdateResult.h"
#include "GameCore/PlayerId.h"
#include "Input/PlayerInput.h"
#include "Utilities/FrameType.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"
#include "Utilities/Containers/FlexArray.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    template <typename SnapshotType>
    class OldRollbackManager {
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
        RollbackCommunicationHandler communicationHandler;
        RollbackSnapshotManager<SnapshotType> snapshotManager;
        InputPredictor inputPredictor;

        // Settings which aren't expected to change mid-game
        bool isInitialized = false;
        FrameType currentInputDelay = OldRollbackStaticSettings::OnlineInputDelay; // NOTE: Assuming both players have same input delay
        bool isLocalPlayer1 = true; // For now hardcoding only two players
        bool isMultiplayerGame = false;
        
        // Various normal processing state
        OldRollbackManagerGameState gameState = {};
        // Technically the below state is part of "GameState" but we should never need the data in snapshots (...supposedly)
        FlexArray<PlayerInput, INPUTS_HISTORY_SIZE> predictedInputs = {};
        bool needToRollback = false;
        FrameType frameToRollbackTo = 0;
    
    public:
        OldRollbackManager() {
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
                return RollbackUpdateResult::ProceedNormally();
            }

            // Only handle the input for a given frame once
            // Eg, even if we're skipping this frame, then store the input and communicate it to MP session (if any)
            if (currentFrame == gameState.latestLocalStoredInputFrame + 1) {
                handleLocalFrameInput(currentFrame, localPlayerInput);
            }

            // If lockstep setting on, then wait for input from other player before proceeding
            // This acts as a quick lockstep implementation test as well as a good way to test waiting logic
            if (OldRollbackStaticSettings::UseLockstep && isMultiplayerGame && gameState.latestRemotePlayerFrame < currentFrame) {
                // logger.logInfoMessage(
                //     "RollbackManager::doFrameUpdate",
                //     "Skipping frame to be in lockstep. Current frame: " + std::to_string(currentFrame)
                //         + ", Remote frame: " + std::to_string(latestRemotePlayerFrame)
                // );
                return RollbackUpdateResult::WaitFrame();
            }

            // Sanity check: If this isn't strictly one frame more than the previous frame, then something went wrong
            if (gameState.latestLocalFrame != 0 && currentFrame != gameState.latestLocalFrame + 1) {
                logger.logWarnMessage(
                    "RollbackManager::doFrameUpdate",
                    "Unexpected currentFrame value! latestLocalFrame: " + std::to_string(gameState.latestLocalFrame)
                        + ", currentFrame: " + std::to_string(currentFrame)
                );
            }
            gameState.latestLocalFrame = currentFrame;

            if (needToRollback) {
                // Reset flag as assuming rollback will be handled immediately
                needToRollback = false;
                
                // Allow caller to handle actual rollback (restoring snapshot and recalculating state up to current frame)
                return  RollbackUpdateResult::Rollback(frameToRollbackTo);
            }
            
            return RollbackUpdateResult::ProceedNormally();
        }

        void onGameFrameEnd(SnapshotType& snapshot) {
            snapshotManager.StoreSnapshot(gameState.latestLocalFrame, snapshot);
        }

        /// <summary>Retrieves input for a given frame. Assumes doFrameUpdate() has been called for every frame already</summary>
        /// <param name="playerId">Player to retrieve inputs for</param>
        /// <param name="frameToRetrieveInputsFor">
        /// Frame to retrieve inputs for. Must be <= current frame and within max rollback range
        /// </param>
        /// <returns>Intended player input for given player on given frame</returns>
        PlayerInput getPlayerInput(const PlayerId& playerId, FrameType frameToRetrieveInputsFor) {
            if (frameToRetrieveInputsFor > gameState.latestLocalFrame) {
                logger.logWarnMessage(
                    "RollbackManager::getInput",
                    "Provided retrieval frame greater than latest frame"
                );
                return {};
            }

            FrameType frameOffset = gameState.latestLocalFrame - frameToRetrieveInputsFor;
            if (frameOffset > OldRollbackStaticSettings::MaxRollbackFrames) {
                logger.logWarnMessage(
                    "RollbackManager::getInput",
                    "Provided retrieval frame greater than max rollback range"
                );
                return {};
            }

            frameOffset += currentInputDelay; // Simple way to apply arbitrary input delay

            if (isRemotePlayer(playerId) && frameToRetrieveInputsFor > gameState.latestRemotePlayerFrame) {
                PlayerInput predictedInput = inputPredictor.predictInput(
                    frameToRetrieveInputsFor, gameState.latestRemotePlayerFrame, getRemotePlayerInputBuffer());

                // Store predicted input so we can check prediction vs real input later on to determine if need to rollback
                // And yeah, given current predictor's implementation we don't need an entire array as prediction for entire
                //     prediction period. However, this implementation is independent of InputPredictor and thus using this
                predictedInputs.Add(predictedInput);

                return predictedInput;
            }
            
            switch(playerId.playerSpot) {
                case PlayerSpot::Player1:
                    return gameState.inputBufferForPlayer1.Get(frameOffset);
                case PlayerSpot::Player2:
                    return gameState.inputBufferForPlayer2.Get(frameOffset);

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

        const SnapshotType& retrieveSnapshot(FrameType frameToRetrieveSnapshotFor) {
            return snapshotManager.GetSnapshot(frameToRetrieveSnapshotFor);
        }

        // FUTURE: Perhaps rename to something akin to "storeSnapshotData" for clarity vs other snapshot behavior
        void storeSnapshot(OldRollbackManagerGameState& output) const {
            output = gameState;
        }

        void restoreSnapshot(const OldRollbackManagerGameState& snapshot) {
            gameState = snapshot;
        }

    private:
        void resetState() {
            isInitialized = false; // Just in case but shouldn't be necessary
            gameState.latestLocalFrame = 0;
            gameState.latestRemotePlayerFrame = 0;
            
            // Prefill input buffer with input delay's worth of data
            // This will guarantee input buffer has enough data to "look back" at the beginning of the game
            //      FUTURE: Note that the for loop may not be necessary depending on what `= {}` does exactly
            //      FUTURE: Also prefilling with enough data for INPUTS_HISTORY_SIZE. Either should explicitly choose max value
            //              or pick a better approach
            gameState.inputBufferForPlayer1 = {};
            gameState.inputBufferForPlayer2 = {};
            for (FrameType i = 0; i < OldRollbackStaticSettings::MaxRollbackFrames; i++) {
                gameState.inputBufferForPlayer1.Add({});
                gameState.inputBufferForPlayer2.Add({});
            }
        }

        OldRollbackInputBuffer& getLocalPlayerInputBuffer() {
            if (isLocalPlayer1) {
                return gameState.inputBufferForPlayer1;
            }
            else {
                return gameState.inputBufferForPlayer2;
            }
        }

        OldRollbackInputBuffer& getRemotePlayerInputBuffer() {
            if (isLocalPlayer1) {
                return gameState.inputBufferForPlayer2;
            }
            else {
                return gameState.inputBufferForPlayer1;
            }
        }

        void handleLocalFrameInput(FrameType currentFrame, const PlayerInput& localPlayerInput) {
            if (currentFrame != gameState.latestLocalStoredInputFrame + 1) { // Sanity check. Likely extraneous but nice to have
                logger.logWarnMessage(
                    "RollbackManager::handleLocalFrameInput",
                    "Unexpected current frame! Current frame: " + std::to_string(currentFrame)
                        + ", latestLocalStoredInputFrame: " + std::to_string(gameState.latestLocalStoredInputFrame)
                );
                return; // Don't continue as storing additional data will break input buffer frame tracking assumptions
            }
            
            // Store player input to be retrieved on the appropriate frame (which may be a latter frame due to input lag)
            OldRollbackInputBuffer& localInputsBuffer = getLocalPlayerInputBuffer();
            localInputsBuffer.Add(localPlayerInput);

            if (isMultiplayerGame) {
                communicationHandler.sendInputsToRemotePlayer(currentFrame, localInputsBuffer);
            }
            
            gameState.latestLocalStoredInputFrame = currentFrame;
        }

        void handleInputUpdateFromConnectedPlayer(const InputUpdateMessage& inputUpdateMessage) {
            if (inputUpdateMessage.updateFrame <= gameState.latestRemotePlayerFrame) {
                logger.logWarnMessage(
                    "RollbackManager::handleInputUpdateFromConnectedPlayer",
                    "Received older frame input: " + std::to_string(inputUpdateMessage.updateFrame)
                );
                return;
            }

            FrameType amountOfNewInputs = inputUpdateMessage.updateFrame - gameState.latestRemotePlayerFrame;
            if (amountOfNewInputs > OldRollbackStaticSettings::MaxRollbackFrames) {
                logger.logWarnMessage(
                    "RollbackManager::handleInputUpdateFromConnectedPlayer",
                    "Received input for frame outside rollback window. Frame: " + std::to_string(inputUpdateMessage.updateFrame)
                );
                // TODO: End game or otherwise resolve the inconsistency!
                return;
            }

            if (!predictedInputs.IsEmpty()) {
                // Sanity check: Predicted inputs should be used every frame that goes beyond latestRemotePlayerFrame
                if (predictedInputs.GetSize() != gameState.latestLocalFrame - gameState.latestRemotePlayerFrame) {
                    logger.logWarnMessage(
                               "RollbackManager::handleInputUpdateFromConnectedPlayer",
                               "predictedInputs.getSize() != drift size! \
                                    , predictions size: " + std::to_string(predictedInputs.GetSize()) +
                                    ", local frame: " + std::to_string(gameState.latestLocalFrame) +
                                    ", latestRemotePlayerFrame: " + std::to_string(gameState.latestRemotePlayerFrame)
                            );
                    predictedInputs = {};
                    return;
                }
                
                // Calculate if we need to rollback (given a prior input update didn't already determine we need to rollback) 
                if (!needToRollback) {
                    // Check if any predicted inputs were inaccurate and thus if we need to rollback
                    // Note that need to start with the earliest prediction as need to rollback to earliest inaccurate prediction
                    for (uint32_t i = 0; i < amountOfNewInputs && i < predictedInputs.GetSize(); i++) {
                        FrameType predictionFrame = gameState.latestLocalFrame - (predictedInputs.GetSize() - 1 - i);
                        if (predictionFrame > inputUpdateMessage.updateFrame) { // Sanity check
                            logger.logWarnMessage(
                               "RollbackManager::handleInputUpdateFromConnectedPlayer",
                               "Somehow predictionFrame > message's updateFrame! i: " + std::to_string(i) +
                                    ", pred frame: " + std::to_string(predictionFrame) +
                                    ", local frame: " + std::to_string(gameState.latestLocalFrame) +
                                    ", msg frame: " + std::to_string(inputUpdateMessage.updateFrame)
                            );
                            break;
                        }
                    
                        FrameType remoteInputOffset = inputUpdateMessage.updateFrame - predictionFrame; // index 0 is the latest frame so go backwards from there
                        if (predictedInputs.Get(i) != inputUpdateMessage.playerInputs[remoteInputOffset]) {
                            needToRollback = true;
                            frameToRollbackTo = predictionFrame;
                            break; // We found the earliest misprediction so no need to check predictions further
                        }
                    }
                }
                
                // Clear out predicted inputs since no longer need em (regardless if rolling back or if they were accurate)
                if (predictedInputs.GetSize() <= amountOfNewInputs) { // Do we have data for all our predictions?
                    predictedInputs = {};
                }
                else { // Otherwise remove only the predictions we have data for and thus no longer need
                    // FlexArray (currently) doesn't support removing elements while retaining order
                    // Thus copy over remaining data into a new array then copy that into the original array
                    // FUTURE: Make this more efficient...? Is this even worth worrying about at all?
                    FlexArray<PlayerInput, INPUTS_HISTORY_SIZE> uncheckedPredictions;
                    for (uint32_t i = amountOfNewInputs; i < predictedInputs.GetSize(); i++) {
                        uncheckedPredictions.Add(predictedInputs.Get(i));
                    }
                    predictedInputs = uncheckedPredictions;
                }
            }

            // Add missing data
            // FUTURE: Have two separate variables here, INPUTS_HISTORY_SIZE and MaxRollbackFrames. Right now they're equal,
            //          but in future they could be different. At risk here to have a bug when those don't equal
            OldRollbackInputBuffer& remoteInputsBuffer = getRemotePlayerInputBuffer();
            for (FrameType i = 0; i < amountOfNewInputs; i++) {
                // index 0 is the latest frame so add backwards
                const PlayerInput& remotePlayerInput = inputUpdateMessage.playerInputs.at(amountOfNewInputs - i);
                remoteInputsBuffer.Add(remotePlayerInput);
            }

            gameState.latestRemotePlayerFrame = inputUpdateMessage.updateFrame;
        }

        bool isRemotePlayer(const PlayerId& playerId) {
            if (!isMultiplayerGame) {
                return false;
            }

            if (isLocalPlayer1) {
                return playerId.playerSpot == PlayerSpot::Player2;
            }
            else {
                return playerId.playerSpot == PlayerSpot::Player1;
            }
        }
    };
}
