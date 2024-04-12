#pragma once

#include "Context/FrameRate.h"
#include "GameCore/PlayerSpot.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    struct RollbackSettings {
        bool isOnlineSession = false;
        
        // Expected to be no less than 0 and no greater than max number in PlayerSpot
        uint8_t totalPlayers = 0;
        // Which "spot" is locally controlled player using? 
        PlayerSpot localPlayerSpot = PlayerSpot::Player4;
        // If playing multiplayer, then which spot represents the host player?
        //      Intended for rollback "polish" features, such as 2+ player time syncing.
        PlayerSpot hostPlayerSpot = PlayerSpot::Player4;
        
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
        // Similar to above, this is intended to represent storing data for current frame, 10 frames that can rollback to,
        //      plus one more older frame beyond the rollback window.
        //      This may be useful for, say, verified (post-rollback window) frame processing. Such as desync detection.
        static constexpr FrameType kTwoMoreThanMaxRollbackFrames = kOneMoreThanMaxRollbackFrames + 1;

        // Easy access to calculation for max "buffer" windows for all relevant rollback windows
        // Size includes rollback window + positive input delay max value + 1 for "current" frame
        // (Note that no need to explicitly account for negative input delay
        static constexpr FrameType kMaxBufferWindow = kMaxRollbackFrames + kMaxInputDelay + 1;

        // How often should "time quality" (time sync) messages be sent to other players?
        //      Don't want too fast as pointless to adjust so quickly (just extra noise), but not too slow as time drift
        //      may build up. Especially if time quality messages are dropped (due to being sent via "UDP")
        static constexpr FrameType kTimeQualityReportFrequency = FrameRate::FromSeconds(fp{1}); // Current time sync duration is 3s so about 3x as fast to cover potential packet drops
        // How often should desync detection checksums occur?
        //      Note that this is expected to be fairly infrequent as checksums are fairly expensive and desyncs are
        //          expected to be rare.
        //      In addition, we're expecting to send these checksums via "reliable but unordered" packets (mix of TCP + UDP)
        //          so being too quick without changing that expectation elsewhere may cause quite a bit of needless logging.
        //      If DO want to compute checksums very frequent, note that likely need to update RollbackDesyncChecker to
        //          either store multiple checksums (if using UDP + receive out of order) or to accept throwing out older
        //          checksums if packets come in out of order or even get dropped entirely.
        static constexpr FrameType kDesyncDetectionFrequency = FrameRate::FromSeconds(fp{1}); // No significant thought put into 1 second frequency
    }; 
}
