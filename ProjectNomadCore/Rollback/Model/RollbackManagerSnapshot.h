#pragma once

#include "Rollback/RollbackInputManager.h"
#include "Rollback/RollbackSnapshotManager.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    /**
    * Represents snapshot of internal state for RollbackManager itself.
    * Useful for restoring RollbackManager to an arbitrary frame, such as for advanced offline replay playback features.
    *
    * In addition, note that no pointers are used at all and thus this struct freely supports copy + equal operations
    **/
    template <typename SnapshotType>
    struct RollbackManagerSnapshot {
        FrameType lastProcessedFrame = 0;

        // FUTURE: Would be nice not to store the "Managers" as a whole, but this works well for now
        RollbackInputManager inputManager = {};
        RollbackSnapshotManager<SnapshotType> snapshotManager = {};
    };
}
