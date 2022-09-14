#pragma once

#include <cstdint>

#if WITH_ENGINE // Use following necessary includes if in Unreal context
#include "CoreMinimal.h"
#include "InputCommand.generated.h"
#else // Otherwise replace Unreal-specific code as appropriate
#include "Utilities/PlatformSupport/UnrealReplacements.h"
#endif


/**
* Supports abstraction layer between player (or AI) inputs to arbitrary commands.
* 
* Makes life easier thinking in actual "commands"/actions instead of needing to fiddle with often-times arbitrary
* inputs, such as what two buttons together initiate a special attack
**/
UENUM(BlueprintType)
enum class InputCommand : uint8_t {
    Crouch,
    Jump,
    Dash,
    DownDodge,

    Block,
    Grapple,
    Attack,
    SpecialAttack,

    Interact,
    SwitchWeapon,

    ENUM_COUNT // Workaround to not (easily) being able to get # of elements in enum
};