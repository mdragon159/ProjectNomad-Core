#include "FVectorFP.h"

#include <CRCpp/CRC.h>
#include "FPMath.h"

FFixedPoint FVectorFP::GetLength() const {
    return ProjectNomad::FPMath::sqrt(GetLengthSquared());
}

FFixedPoint FVectorFP::operator[](int i) const {
    switch (i) {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        default:
            // TODO: Throw error!
            return ProjectNomad::FPMath::minLimit();
    }
}

bool FVectorFP::IsNear(const FVectorFP& other, const FFixedPoint& positiveErrorRange) {
    return ProjectNomad::FPMath::isNear(x, other.x, positiveErrorRange)
        && ProjectNomad::FPMath::isNear(y, other.y, positiveErrorRange)
        && ProjectNomad::FPMath::isNear(z, other.z, positiveErrorRange);
}

void FVectorFP::CalculateCRC32(uint32_t& resultThusFar) const {
    x.CalculateCRC32(resultThusFar);
    y.CalculateCRC32(resultThusFar);
    z.CalculateCRC32(resultThusFar);
}
