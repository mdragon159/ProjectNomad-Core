#pragma once

#include <functional>

#include "EOS/Model/EOSWrapperStatus.h"

namespace ProjectNomad {
    using NetStatusChangeCallback = std::function<void(EOSWrapperStatus currentNetStatus)>;
    using NetGotSelfIdCallback = std::function<void(const std::string& selfCrossPlatformId)>;
}
