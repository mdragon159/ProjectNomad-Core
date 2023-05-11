#pragma once

#include "CrossPlatformIdWrapper.h"
#include "EpicAccountIdWrapper.h"

namespace ProjectNomad {
    /**
    * Apparently need an external hash function to support custom key types for maps:
    * https://www.geeksforgeeks.org/how-to-create-an-unordered_map-of-user-defined-class-in-cpp/
    **/
    struct EOSHashFunction {
        std::size_t operator()(const CrossPlatformIdWrapper& key) const {
            return key.ToHashValue();
        }
        std::size_t operator()(const EpicAccountIdWrapper& key) const {
            return key.ToHashValue();
        }
    };
}
