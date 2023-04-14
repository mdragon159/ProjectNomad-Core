#pragma once

namespace ProjectNomad {
    class INetEventsSubscriber {
    public:
        virtual ~INetEventsSubscriber() = default;

        virtual void OnEOSInitialized() = 0;
        virtual void OnLoginStatusChanged() = 0;

        // Lobby result callbacks
        virtual void OnLobbyCreationResult(bool didLobbyCreationSucceed) = 0;
        virtual void OnLobbyJoinResult(bool didLobbyJoinSucceed) = 0;
        virtual void OnLobbyLeftOrDestroyed(bool didSucceed) = 0;
        virtual void OnLobbyUpdated() = 0;
        // Lobby action begin confirmation callbacks (eg, to show loading spinner)
        virtual void OnLobbyJoinOrCreateBegin() = 0;
        virtual void OnLobbyLeaveBegin() = 0;
        
    };
}
