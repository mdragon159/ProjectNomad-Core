#pragma once

#if !WITH_ENGINE // For uint8 replacement. Not really necessary but eh, nice to have
#include "Utilities/PlatformSupport/UnrealReplacements.h"
#endif

namespace ProjectNomad {
    enum class GameplayInteractiveUIChoice : uint8 {
        None,
        ChooseOptionA,
        ChooseOptionB,
        ChooseOptionC,
        ChooseOptionD,
        ChooseOptionE,
    };
}