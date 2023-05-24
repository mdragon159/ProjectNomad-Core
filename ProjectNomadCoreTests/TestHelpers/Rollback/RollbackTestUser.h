#pragma once

#include "Rollback/RollbackUser.h"
#include "TestHelpers/TestSnapshot.h"

using namespace ProjectNomad;

class RollbackTestUser : public RollbackUser<TestSnapshot> {
public:
    void GenerateSnapshot(FrameType expectedFrame, TestSnapshot& result) override {}
    void RestoreSnapshot(FrameType expectedFrame, const TestSnapshot& snapshotToRestore) override {}
    bool GetInputForNextFrame(FrameType expectedFrame, CharacterInput& result) override { return true; }
    void ProcessFrame(FrameType expectedFrame, const PlayerInputsForFrame& playerInputs) override {}
    void ProcessFrameWithoutRendering(FrameType expectedFrame, const PlayerInputsForFrame& playerInputs) override {}
    void OnPostRollback() override {}
    void SendTimeQualityReport(FrameType currentFrame) override {}
    void SendLocalInputsToRemotePlayers(FrameType expectedFrame, const InputHistoryArray& playerInputs) override {}
    void OnStallingForRemoteInputs(const RollbackStallInfo& stallInfo) override {}
    void OnInputsExitRollbackWindow(FrameType confirmedFrame) override {}
    ~RollbackTestUser() override = default;
};