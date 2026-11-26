#include "game/framework/core/As3.h"

#include <cstdio>
#include <random>

namespace takeover::as3 {

namespace {
std::mt19937& generator() {
    static std::mt19937 engine{std::random_device{}()};
    return engine;
}
} // namespace

double random() { return std::uniform_real_distribution<double>(0.0, 1.0)(generator()); }

void setRandomSeed(std::uint32_t seed) { generator().seed(seed); }

std::string toString(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value > 0 ? "Infinity" : "-Infinity";
    if (value == std::trunc(value) && std::fabs(value) < 1e15) return std::to_string(static_cast<long long>(value));
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.15g", value);
    return buffer;
}

} // namespace takeover::as3
