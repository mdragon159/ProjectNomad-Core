#pragma once

#include <array>
#include <CRCpp/CRC.h>

#include "InputCommand.h"

namespace ProjectNomad {
    /**
    * TODO Description
    **/
    struct CommandSetList {
        std::array<bool, static_cast<size_t>(InputCommand::ENUM_COUNT)> commandInputs = {};

        void SetCommandValue(InputCommand command, bool value) {
            commandInputs[static_cast<size_t>(command)] = value;
        }

        bool IsCommandSet(InputCommand command) const {
            return commandInputs[static_cast<size_t>(command)];
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(commandInputs.data(), commandInputs.size(), CRC::CRC_32(), resultThusFar);
        }

        auto operator<=>(const CommandSetList& buttonInputs) const = default;
    };
}
