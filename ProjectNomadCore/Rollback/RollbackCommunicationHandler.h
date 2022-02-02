#pragma once
#include "Network/NetworkManagerSingleton.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    class RollbackCommunicationHandler {
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
        NetworkManagerSingleton& networkManager = Singleton<NetworkManagerSingleton>::get();
    
    public:
        bool isMultiplayerGame() {
            return networkManager.isConnectedToPlayer();
        }

        void sendInputsToRemotePlayer(FrameType currentFrame, const InputBuffer& remoteInputs) {
            InputHistoryArray inputHistory;
            for (FrameType i = 0; i < INPUTS_HISTORY_SIZE; i++) {
                // Note that 0th spot is current frame's input
                inputHistory[i] = remoteInputs.get(i);
            }

            InputUpdateMessage updateMessage(currentFrame, inputHistory);
            networkManager.sendMessageToConnectedPlayer(updateMessage, PacketReliability::ReliableOrdered);
        }
    };
}
