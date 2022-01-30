#pragma once

#include <functional>

#include "NetStatus.h"

namespace ProjectNomad {
    using NetStatusChangeCallback = std::function<void(NetStatus currentNetStatus)>;
    using NetGotSelfIdCallback = std::function<void(const std::string& selfCrossPlatformId)>;
}
