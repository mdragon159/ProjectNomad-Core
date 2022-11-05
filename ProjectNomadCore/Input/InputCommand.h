#pragma once

#include <cstdint>
/**
* Supports abstraction layer between player (or AI) inputs to arbitrary commands.
* 
* Makes life easier thinking in actual "commands"/actions instead of needing to fiddle with often-times arbitrary
* inputs, such as what two buttons together initiate a special attack
**/
namespace ProjectNomad {
    enum class InputCommand : uint8_t {
        Crouch,
        Jump,
        Dash,

        Block,
        GrappleAim,
        AttackLight,
        AttackHeavy,

        Interact,
        SwitchWeapon,

        ENUM_COUNT // https://stackoverflow.com/a/14989325/3735890
    };
}