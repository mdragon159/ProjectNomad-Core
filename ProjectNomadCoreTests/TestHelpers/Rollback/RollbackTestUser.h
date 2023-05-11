#pragma once

#include "Rollback/RollbackUser.h"
#include "TestHelpers/TestSnapshot.h"

using namespace ProjectNomad;

class RollbackTestUser : public RollbackUser<TestSnapshot> {
public:
    void GenerateSnapshot(FrameType expectedFrame, TestSnapshot& result) override {}
    void RestoreSnapshot(FrameType expectedFrame, const TestSnapshot& snapshotToRestore) override {}
    CharacterInput GetInputForNextFrame(FrameType expectedFrame) override { return {}; }
    void ProcessFrame(FrameType expectedFrame, const PlayerInputsForFrame& playerInputs) override {}
    void ProcessFrameWithoutRendering(FrameType expectedFrame, const PlayerInputsForFrame& playerInputs) override {}
    void OnPostRollback() override {}
    void SendLocalInputsToRemotePlayers(FrameType expectedFrame, const InputHistoryArray& playerInputs) override {}
    void OnStallingForRemoteInputs() override {}
    void OnInputsExitRollbackWindow(FrameType confirmedFrame) override {}
    ~RollbackTestUser() override = default;
};