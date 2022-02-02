#pragma once
#include "NetMessages.h"

namespace ProjectNomad {
    class INetMessageSubscriber {
    public:
        virtual ~INetMessageSubscriber() = default;

        virtual void onMessageReceivedFromConnectedPlayer(NetMessageType messageType, const std::vector<char>& messageData) = 0;
    };
}
