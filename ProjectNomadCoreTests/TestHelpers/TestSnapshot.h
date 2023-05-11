#pragma once
#include "Rollback/Model/BaseSnapshot.h"
#include "Utilities/FrameType.h"

using namespace ProjectNomad;

class TestSnapshot : public BaseSnapshot {
public:
    uint32_t CalculateChecksum() const override {
        return number;
    }

    FrameType number = 0;
};
