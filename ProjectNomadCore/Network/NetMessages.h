#pragma once
#include "GameCore/PlayerInput.h"
#include "Rollback/Old/OldRollbackStaticSettings.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    // FUTURE: Consider using template metaprogramming magic to automatically generate message id
    enum class NetMessageType : uint8_t {
        INVALID,
        TryConnect,
        AcceptConnection,
        StartGame,
        InputUpdate
    };

    // All messages sent to other players are expected to extend from this type
    struct BaseNetMessage {
        // ASSUMING that this enum will always be at the beginning of any given NetMessage
        // Pretty sure this will always be the case when BaseNetMessage is the base type, but do need to verify in future
        NetMessageType messageType = NetMessageType::INVALID;

        BaseNetMessage() {}
        BaseNetMessage(NetMessageType messageType) : messageType(messageType) {}
    };

    struct InitiateConnectionMessage : BaseNetMessage {
        InitiateConnectionMessage() : BaseNetMessage(NetMessageType::TryConnect) {}
    };
    struct AcceptConnectionMessage : BaseNetMessage {
        AcceptConnectionMessage() : BaseNetMessage(NetMessageType::AcceptConnection) {}
    };
    struct StartGameMessage : BaseNetMessage {
        StartGameMessage() : BaseNetMessage(NetMessageType::StartGame) {}
    };

    // For now, send enough inputs always to fill rollback info
    // FUTURE: Minimize packet size via minimizing PlayerInput size and decreasing this var as appropriate
    static constexpr FrameType INPUTS_HISTORY_SIZE = OldRollbackStaticSettings::MaxRollbackFrames;
    using InputHistoryArray = std::array<PlayerInput, INPUTS_HISTORY_SIZE>;
    struct InputUpdateMessage : BaseNetMessage {
        FrameType updateFrame;
        InputHistoryArray playerInputs; // Index 0 will be given frame's input
        
        InputUpdateMessage(FrameType currentFrame, const InputHistoryArray& inputs)
        : BaseNetMessage(NetMessageType::InputUpdate), updateFrame(currentFrame), playerInputs(inputs) {}
    };
}
