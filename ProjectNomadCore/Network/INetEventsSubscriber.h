#pragma once

#include <vector>
#include "P2PMessages/NetMessageType.h"
#include "EOS/Model/CrossPlatformIdWrapper.h"

namespace ProjectNomad {
    class INetEventsSubscriber {
    public:
        virtual ~INetEventsSubscriber() = default;

        virtual void OnEOSInitialized() {}
        virtual void OnLoginStatusChanged() {}

        /**
        * Callback when receiving a peer-to-peer message from another player.
        * @param senderId - Message sender's id
        * @param messageType - Message identifier which was already retrieved from first byte(s) of message data
        * @param messageData - Unvalidated raw message from other player. Should validate size before using data!
        * @returns true if message handled (and thus no need to propagate further to other handlers), false otherwise.
        **/
        virtual bool HandleReceivedP2PMessage(const CrossPlatformIdWrapper& senderId,
                                              NetMessageType messageType,
                                              const std::vector<char>& messageData) { return false; }

        virtual void OnAllPlayerInfoQueriesCompleted() {}
        virtual void OnReceivedPlayerSpotMapping() {}

        // Lobby result callbacks
        virtual void OnLobbyCreationResult(bool didLobbyCreationSucceed) {}
        virtual void OnLobbyJoinResult(bool didLobbyJoinSucceed) {}
        virtual void OnLobbyLeftOrDestroyed(bool didSucceed) {}
        virtual void OnLobbyUpdated() {}
        // Lobby action begin confirmation callbacks (eg, to show loading spinner)
        virtual void OnLobbyJoinOrCreateBegin() {}
        virtual void OnLobbyLeaveBegin() {}
    };
}
