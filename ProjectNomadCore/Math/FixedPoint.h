#pragma once

#include <string>
#include <fpm/fixed.hpp>

namespace ProjectNomad {

    //using fp = cnl::elastic_scaled_integer<28, -12, int32_t>;
    //using fp = cnl::scaled_integer<int64_t, cnl::power<-24>>;
    
    //using fp = fpm::fixed_16_16; // Under/overflows like crazy

    using fpBaseType = std::int64_t;
    using fp = fpm::fixed<fpBaseType, fpBaseType, 16>; // This works well so far! See following for risks and limitations: https://github.com/MikeLankamp/fpm/issues/23

    //using fp = fpm::fixed<std::int64_t, std::int64_t, 20>; // Grapple + camera bugs... but why?
    //using fp = fpm::fixed<std::int64_t, boost::multiprecision::int128_t, 32>; // Extremely slow!

    // fpm notes:
    // Q24.8 is wildly inaccurate, should use at least 16 bits for fraction for reasonable accuracy of trig operations
    // In addition, fpm does NOT guard against underflow and overflow. Need to be very careful with value ranges
    //          (but still don't have a design in place to help detect those... hmm... debug helpers/catchers?)

    static std::string fpToString(fp value) {
        return std::to_string(static_cast<float>(value));
    }

    inline std::ostream& operator<<(std::ostream& os, const fp& value) {
        os << fpToString(value);
        return os;
    }
}