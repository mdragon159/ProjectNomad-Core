#pragma once
#include "CrossPlatformIdWrapper.h"

namespace ProjectNomad {
    class EOSWrapperInterface {
    public:
        virtual ~EOSWrapperInterface() = default;

        virtual void onLoginSuccess(CrossPlatformIdWrapper loggedInCrossPlatformId) = 0;
        virtual void onMessageReceived(CrossPlatformIdWrapper peerId, const std::vector<char>& messageData) = 0;
    };
}
