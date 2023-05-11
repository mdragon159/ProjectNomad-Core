#pragma once

#include "CharacterInput.h"
#include "GameCore/PlayerSpot.h"
#include "Utilities/Containers/FlexArray.h"

namespace ProjectNomad {
    using PlayerInputsForFrame = FlexArray<CharacterInput, PlayerSpotHelpers::kMaxPlayerSpots>;
}
