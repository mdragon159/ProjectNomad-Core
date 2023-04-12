#pragma once

#include <string>

#include "Utilities/DebugMessage.h"

namespace ProjectNomad {
    enum class NetLogCategory { SimLayer, EosSdk }; // TODO: Set explicit type for all enums
    
    struct NetLogMessage {
        std::string message;
        LogSeverity logSeverity = LogSeverity::Info;
        OutputColor color = OutputColor::White;
        NetLogCategory category = NetLogCategory::SimLayer;
    };
}
