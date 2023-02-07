#pragma once

#include <iostream>
#include <CRCpp/CRC.h>

#include "FixedPoint.h"
#include "FPMath.h"

namespace ProjectNomad {
    class FPVector {
    public:
        fp x;
        fp y;
        fp z;

        FPVector() : x(0), y(0), z(0) {}
        explicit FPVector(const fp& val): x(val), y(val), z(val) {}
        FPVector(fp x, fp y, fp z) : x(x), y(y), z(z) {}

        static FPVector zero() {
            return {fp{0}, fp{0}, fp{0}};
        }

        static FPVector forward() {
            return {fp{1}, fp{0}, fp{0}};
        }

        static FPVector right() {
            return {fp{0}, fp{1}, fp{0}};
        }

        static FPVector up() {
            return {fp{0}, fp{0}, fp{1}};
        }

        static FPVector backward() {
            return {fp{-1}, fp{0}, fp{0}};
        }

        static FPVector left() {
            return {fp{0}, fp{-1}, fp{0}};
        }

        static FPVector down() {
            return {fp{0}, fp{0}, fp{-1}};
        }

        static fp distanceSq(const FPVector& from, const FPVector& to) {
            return (to - from).getLengthSquared();
        }

        static fp distance(const FPVector& from, const FPVector& to) {
            return (to - from).getLength();
        }

        static FPVector directionNotNormalized(const FPVector& from, const FPVector& to) {
            return to - from;
        }

        static FPVector direction(const FPVector& from, const FPVector& to) {
            return directionNotNormalized(from, to).normalized();
        }

        static FPVector midpoint(const FPVector& a, const FPVector& b) {
            return (a + b) / fp{2};
        }

        fp getLengthSquared() const {
            return x * x + y * y + z * z;
        }
        
        fp getLength() const {
            return FPMath::sqrt(getLengthSquared());
        }

        FPVector operator-() const {
            return {-x, -y, -z};
        }

        FPVector operator+(const FPVector& v) const {
            return {x + v.x, y + v.y, z + v.z};
        }

        FPVector operator+=(const FPVector& v) {
            x += v.x;
            y += v.y;
            z += v.z;

            return *this;
        }

        FPVector operator-(const FPVector& v) const {
            return {x - v.x, y - v.y, z - v.z};
        }

        FPVector operator-=(const FPVector& v) {
            x -= v.x;
            y -= v.y;
            z -= v.z;

            return *this;
        }

        FPVector operator*(fp value) const {
            return {x * value, y * value, z * value};
        }

        FPVector operator/(fp value) const {
            return {x / value, y / value, z / value};
        }

        fp operator[](int i) const {
            switch (i) {
            case 0:
                return x;
            case 1:
                return y;
            case 2:
                return z;
            default:
                // TODO: Throw error!
                return FPMath::minLimit();
            }
        }

        FPVector normalized() const {
            auto length = getLength();

            // IDEA: Likely far better to log and/or assert as likely unexpected scenario
            // IDEA: Somehow use LengthSq for this check instead while still being efficient (eg, take square after)
            if (length == fp{0}) {
                return zero();
            }

            return *this / length;
        }

        void normalize() {
            *this = normalized();
        }

        FPVector flipped() const {
            return *this * fp{-1};
        }

        void flip() {
            *this = flipped();
        }

        fp dot(const FPVector& other) const {
            return x * other.x + y * other.y + z * other.z;
        }

        FPVector cross(const FPVector& other) const {
            FPVector result;

            result.x = y * other.z - z * other.y;
            result.y = z * other.x - x * other.z;
            result.z = x * other.y - y * other.x;

            return result;
        }

        bool IsZero() const {
            return x == fp{0} && y == fp{0} && z == fp{0};
        }

        bool isNear(const FPVector& other, const fp& positiveErrorRange) {
            return FPMath::isNear(x, other.x, positiveErrorRange)
                && FPMath::isNear(y, other.y, positiveErrorRange)
                && FPMath::isNear(z, other.z, positiveErrorRange);
        }

        /**
         * Specifically, this returns true if the current vector has a component opposite to the input vector.
         * @param other input vector to compare against
         * @returns true if this vector has a component opposite to input vector 
         */
        bool isOppositeDirectionTo(const FPVector& other) const {
            // Dot product is only negative if vectors are in opposite directions
            return dot(other) < fp{0};
        }

        void CalculateCRC32(uint32_t& resultThusFar) const {
            resultThusFar = CRC::Calculate(&x, sizeof(x), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&y, sizeof(y), CRC::CRC_32(), resultThusFar);
            resultThusFar = CRC::Calculate(&z, sizeof(z), CRC::CRC_32(), resultThusFar);
        }

        std::string toString() const {
            auto floatX = (float)x;
            auto floatY = (float)y;
            auto floatZ = (float)z;

            return "x: " + std::to_string(floatX)
                + " | y: " + std::to_string(floatY)
                + " | z: " + std::to_string(floatZ);
        }
    };

    inline bool operator==(const FPVector& lhs, const FPVector& rhs) {
        return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    inline bool operator!=(const FPVector& lhs, const FPVector& rhs) {
        return !(lhs == rhs);
    }

    inline FPVector operator*(fp lhs, const FPVector& rhs) {
        return rhs * lhs;
    }

    inline std::ostream& operator<<(std::ostream& os, const FPVector& vector) {
        os << "FPVector<" << static_cast<float>(vector.x) << ", "
                        << static_cast<float>(vector.y) << ", "
                        << static_cast<float>(vector.z) << ">";
        return os;
    }

}
