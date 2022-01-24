#pragma once

#include "FixedPoint.h"

/* TODO
* fromQuat
* 
* (Quat::toQuat)
*/

namespace ProjectNomad {
    // Represents rotation as three angles in degrees: roll, pitch, yaw
    // If not familiar with Euler angles, see https://en.wikipedia.org/wiki/Euler_angles
    // Note that Quaternions are typically preferred to represent rotations internally (eg, physics systems)
    // Fun side note/reminder: Rotations are applied in order of yaw (Z), pitch (Y), roll (X)
    class EulerAngles {
    public:
        fp roll;
        fp pitch;
        fp yaw;

        EulerAngles() : roll(0), pitch(0), yaw(0) {}
        EulerAngles(fp roll, fp pitch, fp yaw) : roll(roll), pitch(pitch), yaw(yaw) {}

        static EulerAngles zero() {
            return EulerAngles(fp{0}, fp{0}, fp{0});
        }

        EulerAngles operator-() const {
            EulerAngles result;
            result.roll = -roll;
            result.pitch = -pitch;
            result.yaw = -yaw;

            return result;
        }

        std::string toString() const {
            auto floatRoll = static_cast<float>(roll);
            auto floatPitch = static_cast<float>(pitch);
            auto floatYaw = static_cast<float>(yaw);

            return "roll: " + std::to_string(floatRoll)
                + " | pitch: " + std::to_string(floatPitch)
                + " | yaw: " + std::to_string(floatYaw);
        }
    };

    inline bool operator==(const EulerAngles& lhs, const EulerAngles& rhs) {
        return lhs.roll == rhs.roll && lhs.pitch == rhs.pitch && lhs.yaw == rhs.yaw;
    }

    inline bool operator!=(const EulerAngles& lhs, const EulerAngles& rhs) {
        return !(lhs == rhs);
    }

    inline std::ostream& operator<<(std::ostream& os, const EulerAngles& value) {
        os << "EulerAngles<" << value.toString() << ">";
        return os;
    }
}
