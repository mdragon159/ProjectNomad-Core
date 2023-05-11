#pragma once

#include "BaseNetMessage.h"

namespace ProjectNomad {
    struct InitiateConnectionMessage : BaseNetMessage {
        InitiateConnectionMessage() : BaseNetMessage(NetMessageType::TryConnect) {}
    };
    struct AcceptConnectionMessage : BaseNetMessage {
        AcceptConnectionMessage() : BaseNetMessage(NetMessageType::AcceptConnection) {}
    };


    
    struct PrepareLobbyStartMessage : BaseNetMessage {
        PrepareLobbyStartMessage() : BaseNetMessage(NetMessageType::PrepareLobbyStartMatch) {}
    };
    struct ConfirmedLobbyStartMessage : BaseNetMessage {
        ConfirmedLobbyStartMessage() : BaseNetMessage(NetMessageType::ConfirmedLobbyStartMatch) {}
    };
    struct LoadMapMessage : BaseNetMessage {
        LoadMapMessage() : BaseNetMessage(NetMessageType::LoadMap) {}

        // Various explicit session settings that host dictates
        //      Note: Likely want to refactor this message out of this file. And perhaps add a value validation method?
        //      Need to account for bad data from other players somehow/somewhere, or be able to fallback if SimLayer setup fails.
        uint8_t sessionSeed = 0;
    };
    struct FinishedMapLoadMessage : BaseNetMessage {
        FinishedMapLoadMessage() : BaseNetMessage(NetMessageType::FinishedMapLoad) {}
    };
    struct StartGameplayMessage : BaseNetMessage {
        StartGameplayMessage() : BaseNetMessage(NetMessageType::StartGameplay) {}
    };
}
