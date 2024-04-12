#include "FixedPoint.h"

#include <CRCpp/CRC.h>

void FFixedPoint::CalculateCRC32(uint32_t& resultThusFar) const {
    resultThusFar = CRC::Calculate(&m_value, sizeof(m_value), CRC::CRC_32(), resultThusFar);
}
