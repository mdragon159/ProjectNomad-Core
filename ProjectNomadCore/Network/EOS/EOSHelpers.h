#pragma once
#include <string>

#include "Model/PacketReliability.h"
#include "EOS/Include/eos_common.h"
#include "EOS/Include/eos_p2p_types.h"
#include "Utilities/LoggerSingleton.h"


namespace ProjectNomad {
    /**
    * Contains utility methods related to EOS SDK for use in EOSWrapper and relevant layer code
    **/
    class EOSHelpers {
      public:
        EOSHelpers() = delete;

        static std::string ResultCodeToString(const EOS_EResult& result) {
            return EOS_EResult_ToString(result);
        }

        static EOS_EPacketReliability ConvertPacketReliability(LoggerSingleton& logger, PacketReliability inPacketReliability) {
            switch (inPacketReliability) {
                case PacketReliability::UnreliableUnordered:
                    return EOS_EPacketReliability::EOS_PR_UnreliableUnordered;
                case PacketReliability::ReliableUnordered:
                    return EOS_EPacketReliability::EOS_PR_ReliableUnordered;
                case PacketReliability::ReliableOrdered:
                    return EOS_EPacketReliability::EOS_PR_ReliableOrdered;
                default:
                    logger.LogWarnMessage("Unexpected PacketReliability value, perhaps missing case?");
                    return EOS_EPacketReliability::EOS_PR_ReliableOrdered;
            }
        }
    };
}
