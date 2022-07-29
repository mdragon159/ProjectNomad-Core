#pragma once

namespace ProjectNomad {
    /**
    * Defines callbacks for sending rollback-related messages 
    **/
    class RollbackNetCommunicator {
      public:
        virtual ~RollbackNetCommunicator() = default;

        /** TODO/Ideas:
         * 1. SendCurrentFrameInput
         * 2. FUTURE: Time sync messages
         */
    };
}
