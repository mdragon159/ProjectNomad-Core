#pragma once

#include <string>

#include "Utilities/DebugMessage.h"

namespace ProjectNomad {
    enum class NetLogCategory { SimLayer, EosSdk }; // TODO: Set explicit type for all enums
    
    class NetLogMessage {
        static const OutputColor DEFAULT_COLOR = OutputColor::White;

    public:
        NetLogCategory category = NetLogCategory::SimLayer;
        OutputColor color = DEFAULT_COLOR;
        std::string message;
        
        
        NetLogMessage(std::string message): message(message) {}
        NetLogMessage(std::string message, OutputColor color): color(color), message(message) {}
        
        NetLogMessage(std::string message, NetLogCategory category): category(category), message(message) {}
        NetLogMessage(std::string message, OutputColor color, NetLogCategory category): category(category), color(color), message(message) {}
    };
}
