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

        Guard,
        GrappleAim,
        AttackPrimary,
        AttackSecondary,

        Interact,
        SwitchWeapon,

        UseCoreSpell1,
        UseCoreSpell2,
        UseCoreSpell3,
        UseCoreSpell4,
        
        UseCardSpell1,
        UseCardSpell2,
        UseCardSpell3,
        UseCardSpell4,
        
        CombineSpells1,
        CombineSpells2,
        CombineSpells3,
        CombineSpells4,

        ENUM_COUNT // https://stackoverflow.com/a/14989325/3735890
    };
}