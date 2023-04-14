#pragma once

#include "Model/CrossPlatformIdWrapper.h"
#include "Model/EOSLobbyProperties.h"

namespace ProjectNomad {
    class IEOSWrapperManager {
    public:
        virtual ~IEOSWrapperManager() = default;

        virtual void OnLoginSuccess(const CrossPlatformIdWrapper& loggedInCrossPlatformId) = 0;
        virtual void OnLogoutSuccess() = 0;
        virtual void OnMessageReceived(const CrossPlatformIdWrapper& peerId, const std::vector<char>& messageData) = 0;

        // Lobby result callbacks
        virtual void OnLobbyCreationResult(bool didSucceed, const EOSLobbyProperties& lobbyProperties) = 0;
        virtual void OnLobbyJoinResult(bool didSucceed, const EOSLobbyProperties& lobbyProperties) = 0;
        virtual void OnLobbyLeftOrDestroyed(bool didSucceed) = 0;
        virtual void OnLobbyUpdated(bool didSucceed, const EOSLobbyProperties& lobbyProperties) = 0;
        // Lobby action begin confirmation callbacks (eg, to show loading spinner or such)
        virtual void OnLobbyJoinOrCreateBegin() = 0;
        virtual void OnLobbyLeaveBegin() = 0;
    };
}
