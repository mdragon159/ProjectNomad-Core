#pragma once

// #include <ostream>

#include "FVectorFP.h"
#include "fpm/FixedPoint.h"

#if WITH_ENGINE
#include "CoreMinimal.h"
#include "FQuatFP.generated.h"
#else
#include "Utilities/PlatformSupport/UnrealReplacements.h"
#endif

// Credits: Based on https://github.com/BSVino/MathForGameDevelopers/blob/quaternion-transform/math/quaternion.cpp
// In addition, if reader isn't comfortable with quaternions, highly recommended to watch his series:
// https://www.youtube.com/watch?v=dttFiVn0rvc&list=PLW3Zl3wyJwWNWsJIPZrmY19urkYHXOH3N&index=1
USTRUCT(BlueprintType)
struct THENOMADGAME_API FQuatFP {
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere)
    FFixedPoint w;
    UPROPERTY(EditAnywhere)
    FVectorFP v;

    FQuatFP() : w(FFixedPoint{0}), v(FFixedPoint{0}, FFixedPoint{0}, FFixedPoint{0}) {}
    FQuatFP(FFixedPoint w, FVectorFP vector) : w(w), v(vector) {}

    // Building a quaternion from an axis-angle rotation.
    static FQuatFP fromRadians(const FVectorFP& n, FFixedPoint angleInRadians);
    static FQuatFP fromDegrees(const FVectorFP& n, FFixedPoint angleInDegrees);

    static FQuatFP identity();
    FQuatFP inverted() const;

    // Multiplying two quaternions together combines the rotations.
    FQuatFP operator*(const FQuatFP& q) const;
    // Rotate a vector with this quaternion.
    FVectorFP operator*(const FVectorFP& input) const;

    auto operator<=>(const FQuatFP&) const = default;
    
    void CalculateCRC32(uint32_t& resultThusFar) const;
    std::string ToString() const;
};

// inline std::ostream& operator<<(std::ostream& os, const FPQuat& value) {
//     os << "FPQuat<" << static_cast<float>(value.w) << ", " << value.v.ToString() << ">";
//     return os;
// }