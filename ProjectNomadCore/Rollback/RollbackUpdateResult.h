#pragma once

namespace ProjectNomad {
    enum class RollbackDecision {
        ProceedNormally,
        WaitFrame,
        Rollback
    };
    struct RollbackUpdateResult {
        RollbackDecision rollbackDecision;
    };
}
