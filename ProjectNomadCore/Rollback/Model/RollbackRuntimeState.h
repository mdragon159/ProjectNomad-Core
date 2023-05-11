#pragma once

#include "Rollback/Managers/RollbackInputManager.h"
#include "Rollback/Managers/RollbackSnapshotManager.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    /**
    * Represents snapshot of internal state for RollbackManager itself.
    * Useful for restoring RollbackManager to an arbitrary frame, such as for advanced offline replay playback features.
    *
    * In addition, note that no pointers are used at all and thus this struct freely supports copy + equal operations.
    * Finally, note that this is not *all* RollbackManager state, such as various session settings. Largely just
    *       what changes from frame to frame.
    **/
    template <typename SnapshotType>
    struct RollbackRuntimeState {
        // Should always be one less than next frame to process (including overflow)
        FrameType lastProcessedFrame = std::numeric_limits<FrameType>::max();

        RollbackInputManager inputManager = {};
        RollbackSnapshotManager<SnapshotType> snapshotManager = {};
    };
}
