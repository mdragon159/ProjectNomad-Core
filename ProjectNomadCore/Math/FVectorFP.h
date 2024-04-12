#pragma once

#include <fpm/FixedPoint.h>

#if WITH_ENGINE // Use following necessary includes if in Unreal context
#include "CoreMinimal.h"
#include "FVectorFP.generated.h"
#else // Otherwise replace Unreal-specific code as appropriate
#include "Utilities/PlatformSupport/UnrealReplacements.h"
#endif

USTRUCT(BlueprintType)
struct THENOMADGAME_API FVectorFP {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FFixedPoint x = FFixedPoint(0);
    UPROPERTY(EditAnywhere)
    FFixedPoint y = FFixedPoint(0);
    UPROPERTY(EditAnywhere)
    FFixedPoint z = FFixedPoint(0);

    constexpr FVectorFP() = default;
    constexpr explicit FVectorFP(const FFixedPoint val): x(val), y(val), z(val) {}
    constexpr FVectorFP(const FFixedPoint inX, const FFixedPoint inY, const FFixedPoint inZ)
        : x(inX), y(inY), z(inZ) {}

    static constexpr FVectorFP Zero() {
        return {FFixedPoint{0}, FFixedPoint{0}, FFixedPoint{0}};
    }
    static constexpr FVectorFP Forward() {
        return {FFixedPoint{1}, FFixedPoint{0}, FFixedPoint{0}};
    }
    static constexpr FVectorFP Right() {
        return {FFixedPoint{0}, FFixedPoint{1}, FFixedPoint{0}};
    }
    static constexpr FVectorFP Up() {
        return {FFixedPoint{0}, FFixedPoint{0}, FFixedPoint{1}};
    }
    static constexpr FVectorFP Backward() {
        return {FFixedPoint{-1}, FFixedPoint{0}, FFixedPoint{0}};
    }
    static constexpr FVectorFP Left() {
        return {FFixedPoint{0}, FFixedPoint{-1}, FFixedPoint{0}};
    }
    static constexpr FVectorFP Down() {
        return {FFixedPoint{0}, FFixedPoint{0}, FFixedPoint{-1}};
    }

    static FFixedPoint DistanceSq(const FVectorFP& from, const FVectorFP& to) {
        return (to - from).GetLengthSquared();
    }

    static FFixedPoint Distance(const FVectorFP& from, const FVectorFP& to) {
        return (to - from).GetLength();
    }

    static FVectorFP DirectionNotNormalized(const FVectorFP& from, const FVectorFP& to) {
        return to - from;
    }

    static FVectorFP Direction(const FVectorFP& from, const FVectorFP& to) {
        return DirectionNotNormalized(from, to).Normalized();
    }

    static FVectorFP Midpoint(const FVectorFP& a, const FVectorFP& b) {
        return (a + b) / FFixedPoint{2};
    }

    constexpr FFixedPoint GetLengthSquared() const {
        return x * x + y * y + z * z;
    }
    
    FFixedPoint GetLength() const;

    constexpr FVectorFP operator-() const {
        return {-x, -y, -z};
    }

    constexpr FVectorFP operator+(const FVectorFP& v) const {
        return {x + v.x, y + v.y, z + v.z};
    }

    constexpr FVectorFP operator+=(const FVectorFP& v) {
        x += v.x;
        y += v.y;
        z += v.z;

        return *this;
    }

    constexpr FVectorFP operator-(const FVectorFP& v) const {
        return {x - v.x, y - v.y, z - v.z};
    }

    constexpr FVectorFP operator-=(const FVectorFP& v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;

        return *this;
    }

    constexpr FVectorFP operator*(FFixedPoint value) const {
        return {x * value, y * value, z * value};
    }

    constexpr FVectorFP operator/(FFixedPoint value) const {
        return {x / value, y / value, z / value};
    }

    FFixedPoint operator[](int i) const;
    
    auto operator<=>(const FVectorFP&) const = default;

    FVectorFP Normalized() const {
        auto length = GetLength();

        // IDEA: Likely far better to log and/or assert as likely unexpected scenario
        // IDEA: Somehow use LengthSq for this check instead while still being efficient (eg, take square after)
        if (length == FFixedPoint{0}) {
            return Zero();
        }

        return *this / length;
    }

    void Normalize() {
        *this = Normalized();
    }

    FVectorFP Flipped() const {
        return *this * FFixedPoint{-1};
    }

    void Flip() {
        *this = Flipped();
    }

    constexpr FFixedPoint Dot(const FVectorFP& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    constexpr FVectorFP Cross(const FVectorFP& other) const {
        FVectorFP result;

        result.x = y * other.z - z * other.y;
        result.y = z * other.x - x * other.z;
        result.z = x * other.y - y * other.x;

        return result;
    }

    constexpr bool IsZero() const {
        return x == FFixedPoint{0} && y == FFixedPoint{0} && z == FFixedPoint{0};
    }

    bool IsNear(const FVectorFP& other, const FFixedPoint& positiveErrorRange);

    /**
     * Specifically, this returns true if the current vector has a component opposite to the input vector.
     * @param other input vector to compare against
     * @returns true if this vector has a component opposite to input vector 
     */
    constexpr bool IsOppositeDirectionTo(const FVectorFP& other) const {
        // Dot product is only negative if vectors are in opposite directions
        return Dot(other) < FFixedPoint{0};
    }

    void CalculateCRC32(uint32_t& resultThusFar) const;

    std::string ToString() const {
        return "x: " + x.ToString() + " | y: " + y.ToString() + " | z: " + z.ToString();
    }
};

constexpr FVectorFP operator*(FFixedPoint lhs, const FVectorFP& rhs) {
    return rhs * lhs;
}