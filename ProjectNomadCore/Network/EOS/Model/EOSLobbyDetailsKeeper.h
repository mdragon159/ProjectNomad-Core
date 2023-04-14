#pragma once

#include "EOS/Include/eos_lobby_types.h"

namespace ProjectNomad {
    /**
    * Lobby handle needs to be carefully stored and reused at different points in time.
    * Thus, have a special wrapper to refer to it easily.
    *
    * This is based on SDK sample's LobbyDetailKeeper, which had the following comment:
    *       "Simple RAII wrapper to make sure LobbyDetails handles are released correctly."
    **/
    using EOSLobbyDetailsKeeper = std::shared_ptr<EOS_LobbyDetailsHandle>;

    // Following method is based on SDK sample's MakeLobbyDetailsKeeper()
    // This is necessary to get around the following error when trying to create the shared_ptr directly:
    //      memory(1133): [C4150] deletion of pointer to incomplete type 'EOS_LobbyDetailsHandle'; no destructor called
    inline EOSLobbyDetailsKeeper MakeLobbyDetailsKeeper(EOS_HLobbyDetails lobbyDetails) {
        return EOSLobbyDetailsKeeper(lobbyDetails, EOS_LobbyDetails_Release);
    }
}
