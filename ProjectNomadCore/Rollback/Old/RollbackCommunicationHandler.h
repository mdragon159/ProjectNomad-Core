#pragma once
#include "Network/SimNetworkManager.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"

namespace ProjectNomad {
    class RollbackCommunicationHandler {
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
        SimNetworkManager& networkManager = Singleton<SimNetworkManager>::get();
    
    public:
        bool isMultiplayerGame() {
            return networkManager.IsConnectedToPlayer();
        }

        void sendInputsToRemotePlayer(FrameType currentFrame, const OldRollbackInputBuffer& remoteInputs) {
            // TODO: Only send inputs up to what was last ack'd
            // eg, if other player ack'd up to 5 frames ago, then send the last 5 frames of data only
            // (need to figure out how to flexibly size the struct though, should be fairly straightforward)

            // Create array of inputs to send
            InputHistoryArray inputHistory;
            for (FrameType i = 0; i < INPUTS_HISTORY_SIZE; i++) {
                // Note that 0th spot is current frame's input
                inputHistory[i] = remoteInputs.Get(i);
            }

            // Support "TCP" for lockstep testing, but otherwise use "UDP" for speed
            PacketReliability packetReliability = OldRollbackStaticSettings::UseLockstep ?
                                            PacketReliability::ReliableOrdered : PacketReliability::UnreliableUnordered;

            // Finally send the actual message
            InputUpdateMessage updateMessage(currentFrame, inputHistory);
            networkManager.SendMessageToConnectedPlayer(updateMessage, packetReliability);
        }
    };
}
