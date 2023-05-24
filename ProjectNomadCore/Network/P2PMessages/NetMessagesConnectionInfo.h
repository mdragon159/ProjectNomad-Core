#pragma once

#include "BaseNetMessage.h"
#include "Utilities/FrameType.h"

namespace ProjectNomad {
    // Send message regarding current "time quality". This is a combination of time sync frame info and ping-pong (ping)
    // message.
    struct TimeQualityReportMessage : BaseNetMessage {
        FrameType currentFrame = 0;
        uint64_t ping = 0;
        
        TimeQualityReportMessage(FrameType inCurrentFrame, uint64_t inCurTimeInMilliSec)
        : BaseNetMessage(NetMessageType::TimeQualityReport), currentFrame(inCurrentFrame), ping(inCurTimeInMilliSec) {}
    };

    struct TimeQualityResponseMessage : BaseNetMessage {
        uint64_t pong = 0; // Ping-pong message style is to send back the input so peer knows their ping or rather RTT (round trip time)
        
        explicit TimeQualityResponseMessage(const TimeQualityReportMessage& reportMessage)
        : BaseNetMessage(NetMessageType::TimeQualityResponse), pong(reportMessage.ping) {}
    };
}
