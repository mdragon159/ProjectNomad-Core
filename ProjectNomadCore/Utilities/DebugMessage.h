#pragma once

#include <string>
#include "Math/FixedPoint.h"
#include "Math/FPQuat.h"
#include "Math/FPVector.h"

namespace ProjectNomad {
    enum class MessageType { Text, Box, Sphere, Capsule };
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
            return createTextMessage(displayLength, outputLocation, message, DEFAULT_COLOR);
        }

        static DebugMessage createTextMessage(fp displayLength,
                                                OutputLocation outputLocation,
                                                const std::string& message,
                                                OutputColor outputColor) {
            DebugMessage result(MessageType::Text, displayLength);

            result.mOutputLocation = outputLocation;
            result.mTextMessage = message;
            result.mOutputColor = outputColor;

            return result;
        }

        static DebugMessage createBoxMessage(fp displayTime,
                                             const FPVector& drawLocation,
                                             const FPVector& boxExtents,
                                             const FPQuat& boxRotation) {
            return createBoxMessage(displayTime, drawLocation, boxExtents, boxRotation, DEFAULT_COLOR);
        }

        static DebugMessage createBoxMessage(fp displayTime,
                                             const FPVector& drawLocation,
                                             const FPVector& boxExtents,
                                             const FPQuat& boxRotation,
                                             OutputColor outputColor) {
            DebugMessage result(MessageType::Box, displayTime);

            result.mDrawLocation = drawLocation;
            result.mBoxExtents = boxExtents;
            result.mRotation = boxRotation;
            result.mOutputColor = outputColor;

            return result;
        }

        static DebugMessage createSphereMessage(fp displayTime, const FPVector& sphereCenter, fp sphereRadius) {
            return createSphereMessage(displayTime, sphereCenter, sphereRadius, DEFAULT_COLOR);
        }

        static DebugMessage createSphereMessage(fp displayTime,
                                                const FPVector& sphereCenter,
                                                fp sphereRadius,
                                                OutputColor outputColor) {
            DebugMessage result(MessageType::Sphere, displayTime);

            result.mDrawLocation = sphereCenter;
            result.mRadius = sphereRadius;
            result.mOutputColor = outputColor;

            return result;
        }

        static DebugMessage createCapsuleMessage(fp displayTime, const FPVector& center, fp radius, fp halfHeight, const FPQuat& rotation) {
            return createCapsuleMessage(displayTime, center, radius, halfHeight, rotation, DEFAULT_COLOR);
        }

        static DebugMessage createCapsuleMessage(fp displayTime,
                                                const FPVector& center,
                                                fp radius,
                                                fp halfHeight,
                                                const FPQuat& rotation,
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
        FPVector mDrawLocation;
        FPQuat mRotation;
        fp mRadius;
        
        // Text message values
        OutputLocation mOutputLocation = OutputLocation::Log;
        std::string mTextMessage;

        // Draw box values
        FPVector mBoxExtents;

        // Draw capsule values
        fp mHalfHeight;
    };
}
