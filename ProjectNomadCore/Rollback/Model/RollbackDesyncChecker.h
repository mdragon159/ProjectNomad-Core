#pragma once

#include "Utilities/FrameType.h"
#include "Utilities/LoggerSingleton.h"

namespace ProjectNomad {
    /**
    * Encapsulates data storage and sanity checks surrounding multiplayer match desync detection, which revolves around
    * simply taking verified frame checksums from multiple players and comparing them.
    *
    * Assumptions made in this class regarding desync check process:
    *    0. This class will only be used for multiplayer matches
    *    1. Checksum desync checks are pretttty infrequent, such that always expecting to complete a desync check
    *         before a new desync check for a new frame starts.
    *    2. Checksum desync messages are sent reliably over network. ie, we should always receive checksum messages
    *         from all parties for any given frame.
    *    3. For initial implementation simplicity: Regular checksums are sent to all peers, but only non-host peers
    *         are expected to compare themselves to the host
    *    4. Only *verified* frame checksums will be given to this class (and sent between players).
    *         ie, we only care about desync detection for a fully "confirmed" or "verified" frame that won't change.
    *         Which is either when a frame exits rollback window or we have all inputs for the given frame
    **/
    class RollbackDesyncChecker {
      public:
        void ProvideRemoteHostChecksum(LoggerSingleton& logger, FrameType targetFrame, uint32_t checksum) {
            // Is this for an entirely new frame?
            SetupForNewFrameIfNecessary(logger, targetFrame);

            // Sanity check that didn't already receive this frame's info and somehow receiving this again
            if (mHaveRemoteHostChecksum) {
                logger.LogWarnMessage(
                    "Ignoring as already received remote player's checksum for this frame! Provided frame: "
                    + std::to_string(targetFrame)
                );
                return;
            }

            // Store checksum for later desync checking
            mRemoteHostChecksum = checksum;
            mHaveRemoteHostChecksum = true;
        }

        void ProvideLocalHostChecksum(LoggerSingleton& logger, FrameType targetFrame, uint32_t checksum) {
            // Is this for an entirely new frame?
            SetupForNewFrameIfNecessary(logger, targetFrame);

            // Sanity check that didn't already receive this frame's info and somehow receiving this again
            if (mHaveLocalChecksum) {
                logger.LogWarnMessage(
                    "Ignoring as already received local player's checksum for this frame! Provided frame: "
                    + std::to_string(targetFrame)
                );
                return;
            }

            // Store checksum for later desync checking
            mLocalChecksum = checksum;
            mHaveLocalChecksum = true;
        }

        bool IsResultForCurrentTargetFrameReady() const {
            return mHaveRemoteHostChecksum && mHaveLocalChecksum;
        }
        bool DidDesyncOccur() {
            mWasDesyncCheckedForCurrentTargetFrame = true;
            
            return mRemoteHostChecksum == mLocalChecksum;
        }

      private:
        void SetupForNewFrameIfNecessary(LoggerSingleton& logger, FrameType targetFrame) {
            // Nothing to do if already on necessary frame
            if (targetFrame == mTargetFrame) {
                return;
            }
            
            // Sanity check: Expecting checks for strictly increasing frames.
            //      (Frame overflow is not expected to be a realistic case ever due to session length)
            if (targetFrame < mTargetFrame) {
                logger.LogWarnMessage(
                    "New target frame is less than old stored target frame! Old stored target frame: "
                    + std::to_string(mTargetFrame) + ", new frame: "  + std::to_string(targetFrame)
                );
                // No need to early return just in case overflow does happen. Low chance of bad side effects in current implementation
            }

            // Sanity check: Desync check is expected to be used for every target frame before moving on
            if (!mWasDesyncCheckedForCurrentTargetFrame) {
                logger.LogWarnMessage(
                    "Desync was not checked for old target frame before moving on to new frame! Old stored target frame: "
                    + std::to_string(mTargetFrame) + ", new frame: "  + std::to_string(targetFrame)
                );
                // Again, no need to early return and break things. Just a concern that should be double checked if/when it occurs
            }

            /// Finally reset state for the new target frame
            // Simple robust "trick" to reset all potential values back to defaults, as defaults are intended to
            //      represent state when new target frame is first set
            *this = {};

            mTargetFrame = targetFrame;
        }
        
        FrameType mTargetFrame = 0; // Expecting to do desync check for first frame
        bool mHaveRemoteHostChecksum = false;
        bool mHaveLocalChecksum = false;
        bool mWasDesyncCheckedForCurrentTargetFrame = false;
        
        uint32_t mRemoteHostChecksum = 0;
        uint32_t mLocalChecksum = 0;
    };
}
