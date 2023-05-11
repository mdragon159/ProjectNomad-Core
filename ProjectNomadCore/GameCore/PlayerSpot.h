#pragma once

#include <cstdint>
#include <type_traits>

namespace ProjectNomad {
    // Manually add an enum for each possible spot value so have neat way to pass around identifier for player spot.
    //      Could try to do some macro magic to fill this up according to the constant.
    enum class PlayerSpot : uint8_t {
        Player1,
        Player2,
        Player3,
        Player4,
        // NOTE: If add any value, then update constant below + search for all switch case usages of enum and update those places as well
    };

    class PlayerSpotHelpers {
      public:
        PlayerSpotHelpers() = delete;

        // Little shortcut utility function. Created so any downstream code trying to do this doesn't mess up 
        static constexpr uint8_t GetMaxPlayerSpotEnumValue() {
            return kMaxPlayerSpots - 1; // enum should always match the constant but enum first value starts at 0
        }

        // Checks if the provided total player count is within range
        template <typename IntegralType> // Use template so don't need to worry about loss of precision during checks (eg, size_t to uint8_t)
        static constexpr bool IsInvalidTotalPlayers(const IntegralType totalPlayers) {
            static_assert(std::is_integral_v<IntegralType>, "IntegralType must be an integral type");
            return totalPlayers < 1 || totalPlayers > kMaxPlayerSpots;
        }
        // Returns true if player spot is for a spot outside current total # of players
        //      Ex: Returns true if totalPlayers = 2 and target spot is Player 3 or 4
        static constexpr bool IsPlayerSpotOutsideTotalPlayers(const uint8_t totalPlayers, PlayerSpot targetSpot) {
            auto targetSpotAsNum = static_cast<uint8_t>(targetSpot);
            return targetSpotAsNum + 1 > totalPlayers; // Add 1 as enum starts at 0 
        }

        // Changing this constant and everything else in this file should nearly instantly change max # of players supported
        static constexpr uint8_t kMaxPlayerSpots = 4;
    };
}
