#pragma once

namespace ProjectNomad {
    class INetEventsSubscriber {
    public:
        virtual ~INetEventsSubscriber() = default;

        virtual void OnEOSInitialized() = 0;
        virtual void OnLoginStatusChanged() = 0;

        virtual void OnLobbyCreationResult(bool didLobbyCreationSucceed) = 0;
        virtual void OnLobbyLeftOrDestroyed(bool didSucceed) = 0;
    };
}
