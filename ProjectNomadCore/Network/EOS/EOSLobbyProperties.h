#pragma once
#include <string>

#include "Context/CoreContext.h"
#include "EOS/Include/eos_lobby_types.h"

namespace ProjectNomad {
    class EOSLobbyProperties {
      public:
        bool TryInit(LoggerSingleton& logger, EOS_LobbyId inputId) {
            if (inputId == nullptr) {
                logger.addWarnNetLog(
                    "EOSLobbyProperties::InitFromLobbyHandle",
                    "Input id is nullptr!"
                );
                return false;
            }

            mLobbyId = inputId;

            // TODO: Any other properties to copy. See FLobby::InitFromLobbyHandle from SDK samples
            //  Eg, lobby owner + members, permissions, etc

            return true;
        }

        const std::string& GetId() const {
            return mLobbyId;
        }

      private:
        std::string mLobbyId = "";
    };
}
