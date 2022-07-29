#pragma once
#include <cstdint>

namespace ProjectNomad {
    /**
    * Internal running "mode" for RollbackManager
    **/
    enum class RollbackManagerMode : uint8_t {
        NotStarted, Running
    };
}
