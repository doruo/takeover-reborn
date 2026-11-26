// As3.h - ActionScript/Haxe number semantics, so formulas read like the
// original and give the same results.
//
//   as3::toInt(x)   int(x): truncates toward zero; NaN/Infinity give 0, and
//                   values outside 32 bits wrap like the AVM2 does.
//   as3::round(x)   Math.round: halves go up (-2.5 -> -2, 2.5 -> 3).
//   as3::random()   Math.random: uniform in [0, 1).
#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace takeover::as3 {

    inline int toInt(double value) {
        if (!std::isfinite(value))
            return 0;

        const double truncated = std::trunc(value);
        if (truncated >= -2147483648.0 && truncated <= 2147483647.0)
            return static_cast<int>(truncated);

        // ECMAScript ToInt32: modulo 2^32, then into the signed range.
        double wrapped = std::fmod(truncated, 4294967296.0);
        if (wrapped < 0)
            wrapped += 4294967296.0;

        return static_cast<int>(static_cast<std::uint32_t>(wrapped));
    }

    inline std::uint32_t toUint(double value) { return static_cast<std::uint32_t>(toInt(value)); }

    inline double round(double value) { return std::floor(value + 0.5); }

    /// Math.random(). Seeded from the clock unless setRandomSeed() is called.
    double random();
    void setRandomSeed(std::uint32_t seed);

    /// Number to string like AS3's String(n): integers without a decimal point.
    std::string toString(double value);

} // namespace takeover::as3
