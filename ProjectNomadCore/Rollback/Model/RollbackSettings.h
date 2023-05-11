#pragma once

#include "GameCore/PlayerSpot.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    struct RollbackSettings {
        // Expected to be no less than 0 and no greater than max number in PlayerSpot
        uint8_t totalPlayers = 1;
        // Which "spot" is locally controlled player using? 
        PlayerSpot localPlayerSpot = PlayerSpot::Player1;

        
        bool useLockstep = false;
        
        bool useSyncTest = false;
        FrameType syncTestFrames = 2;

        // If this is negative then "negative input delay" feature will be used.
        // "Negative input delay" best explained by this: https://medium.com/@yosispring/input-buffering-action-canceling-and-also-forbidden-knowledge-47a3f8a95151
        // In short, game will predict local player's inputs for number of negative input frames, which is useful for
        // 
        int localInputDelay = 3;

        // Additional pure debug settings
        bool logSyncTestChecksums = false;
        bool logChecksumForEveryStoredFrameSnapshot = false;

        bool IsMultiplayerSession() const {
            return totalPlayers > 1;
        }
    };

    struct RollbackStaticSettings {
        static constexpr FrameType kMaxInputDelay = 10;
        
        // Rollback up to this number of frames. Does not impact local "negative input delay" feature.
        // ie, if for some reason trying to test negative input delay locally that's bigger than this number, than
        // the rollback window will simply be the negative input delay.
        static constexpr FrameType kMaxRollbackFrames = 10;
        // Following is a sort of reminder that often need storage for 1 more frame than just # of frames can rollback
        //      (ie, current frame then 10 frames into past)
        static constexpr FrameType kOneMoreThanMaxRollbackFrames = kMaxRollbackFrames + 1;

        // Easy access to calculation for max "buffer" windows for all relevant rollback windows
        // Size includes rollback window + positive input delay max value + 1 for "current" frame
        // (Note that no need to explicitly account for negative input delay
        static constexpr FrameType kMaxBufferWindow = kMaxRollbackFrames + kMaxInputDelay + 1;
    }; 
}
