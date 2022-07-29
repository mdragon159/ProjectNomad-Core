#pragma once
#include "InputBuffer.h"
#include "Utilities/FrameType.h"
#include "Utilities/LoggerSingleton.h"
#include "Utilities/Singleton.h"
#include "Utilities/Containers/RingBuffer.h"

namespace ProjectNomad {
    // Note that SnapshotType should NOT have any pointers as snapshot restoration will be ineffective.
    // This includes std::array and callbacks.

    /// <summary>
    /// Encapsulates snapshot data and related behavior specific to rollbacks.
    /// ie this does not define what a snapshot is nor when to take one, but rather is responsible for:
    /// - Storing snapshots for rollback usage (eg, one per frame and just enough data stored for all rollback needs)
    /// - Retrieving relevant snapshot for any possible rollback usage
    /// </summary>
    template <typename SnapshotType>
    class RollbackSnapshotManager {
        LoggerSingleton& logger = Singleton<LoggerSingleton>::get();
        
        FrameType latestStoredFrame = 0;
        RingBuffer<SnapshotType, INPUT_BUFFER_SIZE> snapshotBuffer;

    public:
        /// <summary>Inserts provided snapshot into buffer</summary>
        /// <param name="currentFrame">Current frame that snapshot is intended for</param>
        /// <param name="snapshot">
        /// Snapshot to insert passed by reference.
        /// Assume that this value will be unusable after the call (due to swap-insert).
        /// </param>
        void storeSnapshot(FrameType currentFrame, SnapshotType& snapshot) {
            // Sanity check: Expecting to store snapshots only once per frame and in order
            if (currentFrame != 0 && currentFrame != latestStoredFrame + 1) {
                logger.logWarnMessage(
                    "RollbackSnapshotManager::storeSnapshot",
                    "Unexpected currentFrame value! latestStoredFrame: " + std::to_string(latestStoredFrame)
                        + ", currentFrame: " + std::to_string(currentFrame)
                );
            }

            snapshotBuffer.swapInsert(snapshot);           
            latestStoredFrame = currentFrame;
        }

        const SnapshotType& getSnapshot(FrameType frameToRetrieveSnapshotFor) {
            if (frameToRetrieveSnapshotFor > latestStoredFrame) {
                logger.logWarnMessage(
                    "RollbackSnapshotManager::getSnapshot",
                    "Provided retrieval frame greater than latest frame"
                );
                return snapshotBuffer.get(0);
            }

            FrameType frameOffset = latestStoredFrame - frameToRetrieveSnapshotFor;
            if (frameOffset > INPUT_BUFFER_SIZE) {
                logger.logWarnMessage(
                    "RollbackSnapshotManager::getSnapshot",
                    "Provided retrieval frame beyond buffer size"
                );
                return snapshotBuffer.get(0);
            }

            return snapshotBuffer.get(frameOffset);
        }
    };
}
