#include "FQuatFP.h"

#include "FPMath.h"

// Building a quaternion from an axis-angle rotation.
// http://youtu.be/SCbpxiCN0U0
FQuatFP FQuatFP::fromRadians(const FVectorFP& n, FFixedPoint angleInRadians) {
    // TODO: Debug assert if n is not normal

    FFixedPoint w = ProjectNomad::FPMath::cosR(angleInRadians / 2);
    FVectorFP v = n * ProjectNomad::FPMath::sinR(angleInRadians / 2);

    return FQuatFP(w, v);
}

FQuatFP FQuatFP::fromDegrees(const FVectorFP& n, FFixedPoint angleInDegrees) {
    return fromRadians(n, ProjectNomad::FPMath::degreesToRadians(angleInDegrees));
}

FQuatFP FQuatFP::identity() {
    return FQuatFP(FFixedPoint{1}, FVectorFP(FFixedPoint{0}, FFixedPoint{0}, FFixedPoint{0}));
}

// http://youtu.be/A6A0rpV9ElA
// ASSUMING unit quaternion! Everything here pretty much assumes that to be fair...
FQuatFP FQuatFP::inverted() const {
    return FQuatFP(w, -v);
}

// Multiplying two quaternions together combines the rotations.
// http://youtu.be/CRiR2eY5R_s
FQuatFP FQuatFP::operator*(const FQuatFP& q) const {
    FQuatFP result;

    result.w = w * q.w + v.Dot(q.v);
    result.v = v * q.w + q.v * w + v.Cross(q.v);

    return result;
}

// Rotate a vector with this quaternion.
// http://youtu.be/Ne3RNhEVSIE
// The basic equation is qpq* (the * means inverse) but we use a simplified version of that equation.
FVectorFP FQuatFP::operator*(const FVectorFP& input) const {
    // Could do it this way:
    /*
    const Quaternion& q = (*this);
    return (q * p * q.Inverted()).v;
    */

    // But let's optimize it a bit instead.
    FVectorFP vCrossInput = v.Cross(input);
    return input + vCrossInput * (2 * w) + v.Cross(vCrossInput) * FFixedPoint{2};
}


void FQuatFP::CalculateCRC32(uint32_t& resultThusFar) const {
    w.CalculateCRC32(resultThusFar);
    v.CalculateCRC32(resultThusFar);
}

std::string FQuatFP::ToString() const {
    auto floatW = static_cast<float>(w);

    return std::to_string(floatW) + ", " + v.ToString();
}
