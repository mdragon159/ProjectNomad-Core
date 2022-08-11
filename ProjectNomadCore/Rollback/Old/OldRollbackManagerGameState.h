#pragma once
#include "InputBuffer.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    struct OldRollbackManagerGameState {
        InputBuffer inputBufferForPlayer1;
        InputBuffer inputBufferForPlayer2;
        
        FrameType latestLocalStoredInputFrame = 0;
        FrameType latestLocalFrame = 0;
        FrameType latestRemotePlayerFrame = 0;
    };
}
