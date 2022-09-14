#pragma once
#include "InputBuffer.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    struct OldRollbackManagerGameState {
        OldRollbackInputBuffer inputBufferForPlayer1;
        OldRollbackInputBuffer inputBufferForPlayer2;
        
        FrameType latestLocalStoredInputFrame = 0;
        FrameType latestLocalFrame = 0;
        FrameType latestRemotePlayerFrame = 0;
    };
}
