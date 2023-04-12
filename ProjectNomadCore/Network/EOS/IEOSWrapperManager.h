#pragma once
#include "CrossPlatformIdWrapper.h"

namespace ProjectNomad {
    class IEOSWrapperManager {
    public:
        virtual ~IEOSWrapperManager() = default;

        virtual void OnLoginSuccess(CrossPlatformIdWrapper loggedInCrossPlatformId) = 0;
        virtual void OnLogoutSuccess() = 0;
        virtual void OnMessageReceived(CrossPlatformIdWrapper peerId, const std::vector<char>& messageData) = 0;

        virtual void OnLobbyCreationResult(bool didSucceed) = 0;
        virtual void OnLobbyLeftOrDestroyed(bool didSucceed) = 0;
    };
}
