#pragma once

namespace ProjectNomad {
    enum class RollbackDecision {
        ProceedNormally,
        WaitFrame,
        Rollback
    };
    struct RollbackUpdateResult {
        RollbackDecision rollbackDecision;
        FrameType frameToRollbackTo = 0;

        static RollbackUpdateResult ProceedNormally() {
            return { RollbackDecision::ProceedNormally };
        }
        static RollbackUpdateResult WaitFrame() {
            return { RollbackDecision::ProceedNormally };
        }
        static RollbackUpdateResult Rollback(FrameType frameToRollbackTo) {
            return { RollbackDecision::ProceedNormally, frameToRollbackTo };
        }
    };
}
