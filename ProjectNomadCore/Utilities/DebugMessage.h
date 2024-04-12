#pragma once

#include <string>
#include "Math/FixedPoint.h"
#include "Math/FQuatFP.h"
#include "Math/FVectorFP.h"

namespace ProjectNomad {
    enum class MessageType { Text, Box, Sphere, Capsule };
    enum class LogSeverity { Info, Warn, Error };
    enum class OutputLocation { Log, Screen, LogAndScreen };
    enum class OutputColor { White, Black, Transparent, Red, Green, Blue, Yellow, Cyan, Magenta, Orange, Purple,
                                Turquoise, Silver, Emerald };

    class DebugMessage {
        static const OutputColor DEFAULT_COLOR = OutputColor::Orange;
        
        DebugMessage(MessageType messageType): mMessageType(messageType) {}
        DebugMessage(MessageType messageType, fp displayTime): mMessageType(messageType), mDisplayTime(displayTime) {}

    public:
        static DebugMessage createLogMessage(const std::string& message) {
            DebugMessage result(MessageType::Text);

            result.mOutputLocation = OutputLocation::Log;
            result.mTextMessage = message;

            return result;
        }

        static DebugMessage createTextMessage(fp displayLength,
                                                OutputLocation outputLocation,
                                                const std::string& message) {
            return createTextMessage(displayLength, outputLocation, message, LogSeverity::Info, DEFAULT_COLOR);
        }

        static DebugMessage createTextMessage(fp displayLength,
                                                OutputLocation outputLocation,
                                                const std::string& message,
                                                LogSeverity logSeverity,
                                                OutputColor outputColor) {
            DebugMessage result(MessageType::Text, displayLength);

            result.mOutputLocation = outputLocation;
            result.mTextMessage = message;
            result.mLogSeverity = logSeverity;
            result.mOutputColor = outputColor;

            return result;
        }

        static DebugMessage createBoxMessage(fp displayTime,
                                             const FVectorFP& drawLocation,
                                             const FVectorFP& boxExtents,
                                             const FQuatFP& boxRotation) {
            return createBoxMessage(displayTime, drawLocation, boxExtents, boxRotation, DEFAULT_COLOR);
        }

        static DebugMessage createBoxMessage(fp displayTime,
                                             const FVectorFP& drawLocation,
                                             const FVectorFP& boxExtents,
                                             const FQuatFP& boxRotation,
                                             OutputColor outputColor) {
            DebugMessage result(MessageType::Box, displayTime);

            result.mDrawLocation = drawLocation;
            result.mBoxExtents = boxExtents;
            result.mRotation = boxRotation;
            result.mOutputColor = outputColor;

            return result;
        }

        static DebugMessage createSphereMessage(fp displayTime, const FVectorFP& sphereCenter, fp sphereRadius) {
            return createSphereMessage(displayTime, sphereCenter, sphereRadius, DEFAULT_COLOR);
        }

        static DebugMessage createSphereMessage(fp displayTime,
                                                const FVectorFP& sphereCenter,
                                                fp sphereRadius,
                                                OutputColor outputColor) {
            DebugMessage result(MessageType::Sphere, displayTime);

            result.mDrawLocation = sphereCenter;
            result.mRadius = sphereRadius;
            result.mOutputColor = outputColor;

            return result;
        }

        static DebugMessage createCapsuleMessage(fp displayTime, const FVectorFP& center, fp radius, fp halfHeight, const FQuatFP& rotation) {
            return createCapsuleMessage(displayTime, center, radius, halfHeight, rotation, DEFAULT_COLOR);
        }

        static DebugMessage createCapsuleMessage(fp displayTime,
                                                const FVectorFP& center,
                                                fp radius,
                                                fp halfHeight,
                                                const FQuatFP& rotation,
                                                OutputColor outputColor) {
            DebugMessage result(MessageType::Capsule, displayTime);

            result.mDrawLocation = center;
            result.mRadius = radius;
            result.mHalfHeight = halfHeight;
            result.mRotation = rotation;
            result.mOutputColor = outputColor;

            return result;
        }


        // Values used across multiple types
        MessageType mMessageType;
        fp mDisplayTime;
        OutputColor mOutputColor = DEFAULT_COLOR;
        FVectorFP mDrawLocation;
        FQuatFP mRotation;
        fp mRadius;
        
        // Text message values
        OutputLocation mOutputLocation = OutputLocation::Log;
        LogSeverity mLogSeverity = LogSeverity::Info;
        std::string mTextMessage;

        // Draw box values
        FVectorFP mBoxExtents;

        // Draw capsule values
        fp mHalfHeight;
    };
}
