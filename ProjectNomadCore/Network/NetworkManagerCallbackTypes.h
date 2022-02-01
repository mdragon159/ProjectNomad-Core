#pragma once

#include <functional>

#include "EOS/EOSWrapperStatus.h"

namespace ProjectNomad {
    using NetStatusChangeCallback = std::function<void(EOSWrapperStatus currentNetStatus)>;
    using NetGotSelfIdCallback = std::function<void(const std::string& selfCrossPlatformId)>;
}
