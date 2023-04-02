#pragma once
#include "Input/CharacterInput.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    /**
    * Defines a "user" (consuming code) which is using the relevant RollbackManager.
    * This essentially defines an interface or rather the necessary set of callbacks to support all
    * rollback-related functionality.
    * @tparam SnapshotType - defines struct used for frame snapshot. "Restoring" this should effectively return to a prior frame
    **/
    template <typename SnapshotType>
    class RollbackUser {
      public:
        virtual ~RollbackUser() = default;

        /**
        * Generates snapshot for start of frame. This will be used with RestoreSnapshot when rollback occurs.
        * @param expectedFrame - frame number of snapshot. Called before frame number is processed (ie, before ProcessFrame
        *                        is called for related frame). Only intended for debug assistance
        * @param result - represents snapshot of current frame. Can be assumed to be "empty"/default initializer state
        **/
        virtual void GenerateSnapshot(FrameType expectedFrame, SnapshotType& result) = 0;

        /**
        * Callback to restore the gameplay state from the provided snapshot.
        * This is the "rollback to previous state" part of the rollback library.
        * @param expectedFrame - frame number of snapshot. Only intended for debug assistance
        * @param snapshotToRestore - represents gameplay snapshot that should be used going forward
        **/
        virtual void RestoreSnapshot(FrameType expectedFrame, const SnapshotType& snapshotToRestore) = 0;

        /**
        * Callback to process gameplay for a single "frame".
        * 
        * Underlying code should remember to move one frame forward.
        * ie, if game state is currently at frame 10, then one call to this method is expected to process frame 10.
        * Thereafter the next call is expected to process frame 11. 
        * @param expectedFrame - frame number that is expected to be processed. Only intended for debug assistance
        * @param localPlayerInput - Input to be used with frame processing
        **/
        virtual void ProcessFrame(FrameType expectedFrame, const CharacterInput& localPlayerInput) = 0;

        /**
        * Identical to ProcessFrame except expecting rendering to not be necessary.
        *
        * This callback is used when a rollback occurs and then need to re-process frames quickly back to back.
        * After a rollback, only this method and OnPostRollback will be called instead of ProcessFrame
        **/
        virtual void ProcessFrameWithoutRendering(FrameType expectedFrame, const CharacterInput& localPlayerInput) = 0;

        /**
        * Called after a rollback occurs and frames are finished re-processing.
        * This is intended to be used for updating rendering after rollback processing is complete.
        **/
        virtual void OnPostRollback() = 0;

        /**
        * Called when inputs leave the "rollback window", ie when there's no chance of them being rolled back and
        * otherwise changed.
        *
        * Intended purpose is writing inputs to a replay file once we're confident they won't change, which is
        * particularly relevant for multiplayer games.
        * @param confirmedFrame - frame that inputs have been fully validated up to (inclusive)
        **/
        virtual void OnInputsExitRollbackWindow(FrameType confirmedFrame) = 0;
    };
}
