#pragma once

#include "InputCommand.h"
#include "Utilities/Containers/NumericBitSet.h"

namespace ProjectNomad {
    /**
    * Simply stores whether any given command is "set"
    **/
    struct CommandSetList {
        static_assert(static_cast<size_t>(InputCommand::ENUM_COUNT) <= 16,
            "Bitset is currently set to 16 bits. If more than 16 commands exist, then this struct must be updated");
        NumericBitSet<uint16_t> commandInputs = {};

        void SetCommandValue(InputCommand command, bool value) {
            commandInputs.SetIndex(ToIndex(command), value);
        }

        bool IsCommandSet(InputCommand command) const {
            return commandInputs.GetIndex(ToIndex(command));
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            commandInputs.CalculateCRC32(resultThusFar);
        }

        uint16_t Serialize() const {
            return commandInputs.GetAllAsNumber();
        }
        void Deserialize(uint16_t serializedInput) {
            commandInputs.SetAllAsNumber(serializedInput);
        }

        auto operator<=>(const CommandSetList& other) const = default;

    private:
        static constexpr uint16_t ToIndex(InputCommand command) {
            return static_cast<size_t>(command);
        }
    };
}
