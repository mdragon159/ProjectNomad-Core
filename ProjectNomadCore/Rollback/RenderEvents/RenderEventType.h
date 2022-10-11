#pragma once

namespace ProjectNomad {
    /**
    * Identifier for what fx is expected for the corresponding RenderEvent.
    * 
    * We *could* easily separate this out into two separate SFX vs VFX enums, but no need atm.
    * Lots of tiny changes that could be done in future around RenderEvent identifiers and thus just KISS
    **/
    enum class RenderEventType : uint8_t {
        INVALID_ID,
    
        SFXHitSwordBasic,
    
        VFXHitSwordBasic,
    };
}