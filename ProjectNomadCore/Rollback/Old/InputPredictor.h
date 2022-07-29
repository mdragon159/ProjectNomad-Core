#pragma once
#include "InputBuffer.h"
#include "Utilities/FrameType.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    class InputPredictor {
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
    
    public:
        PlayerInput predictInput(FrameType frameToRetrieveInputFor,
                                 FrameType latestStoredInputFrame,
                                 const InputBuffer& inputBuffer) {

            // Simply retrieve the latest input we know about
            return inputBuffer.get(0);
        }
    };
}
