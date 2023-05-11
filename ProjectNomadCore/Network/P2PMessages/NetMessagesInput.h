#pragma once

#include <array>

#include "BaseNetMessage.h"
#include "Input/CharacterInput.h"
#include "Utilities/FrameType.h"
#include "Rollback/Model/RollbackSettings.h"

namespace ProjectNomad {
    // TODO: Only send necessary amount of inputs.
    //       Eg, send back and forth confirmed frame for inputs, then send up to that frame of inputs.
    
    // For now, send enough inputs always to fill rollback info
    // FUTURE: Minimize packet size via minimizing PlayerInput size and decreasing this var as appropriate
    static constexpr FrameType kInputsHistorySize = RollbackStaticSettings::kMaxRollbackFrames;
    using InputHistoryArray = std::array<CharacterInput, kInputsHistorySize>;
    
    struct InputUpdateMessage : BaseNetMessage {
        FrameType updateFrame = std::numeric_limits<FrameType>::max();
        InputHistoryArray playerInputs = {}; // Index 0 will be given frame's input

        InputUpdateMessage() : BaseNetMessage(NetMessageType::InputUpdate) {}
        InputUpdateMessage(FrameType currentFrame, const InputHistoryArray& inputs)
        : BaseNetMessage(NetMessageType::InputUpdate), updateFrame(currentFrame), playerInputs(inputs) {}
    };
}
