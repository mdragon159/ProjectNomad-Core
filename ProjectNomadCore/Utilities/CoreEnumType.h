#pragma once

#if WITH_ENGINE
#else // Unreal likes uint8 while standalone C++ likes to use uint8_t. Replace uint8 with uint8_t if not using Unreal
#include "Utilities/PlatformSupport/UnrealReplacements.h"
#endif

namespace ProjectNomad {
    // Using uint8 to assure compatibility with Unreal Blueprints/Editor
    using CoreEnumType = uint8;
}
