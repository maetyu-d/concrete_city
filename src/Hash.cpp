#include "Hash.hpp"

#include <cmath>

namespace {
double fade(double t) {
    return t * t * (3.0 - 2.0 * t);
}

double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}
}

std::uint64_t hashCombine(std::uint64_t a, std::uint64_t b) {
    a ^= b + 0x9e3779b97f4a7c15ULL + (a << 6U) + (a >> 2U);
    a ^= a >> 30U;
    a *= 0xbf58476d1ce4e5b9ULL;
    a ^= a >> 27U;
    a *= 0x94d049bb133111ebULL;
    a ^= a >> 31U;
    return a;
}

std::uint64_t stableHash(std::uint64_t seed, std::int64_t x, std::int64_t y, std::uint64_t salt) {
    auto h = hashCombine(seed, static_cast<std::uint64_t>(x));
    h = hashCombine(h, static_cast<std::uint64_t>(y));
    return hashCombine(h, salt);
}

double hashToUnit(std::uint64_t hash) {
    constexpr double denom = static_cast<double>(UINT64_MAX);
    return static_cast<double>(hash) / denom;
}

double valueNoise2D(std::uint64_t seed, double x, double y, double frequency, std::uint64_t salt) {
    const double sx = x * frequency;
    const double sy = y * frequency;
    const auto x0 = static_cast<std::int64_t>(std::floor(sx));
    const auto y0 = static_cast<std::int64_t>(std::floor(sy));
    const double tx = sx - static_cast<double>(x0);
    const double ty = sy - static_cast<double>(y0);

    const double a = hashToUnit(stableHash(seed, x0, y0, salt));
    const double b = hashToUnit(stableHash(seed, x0 + 1, y0, salt));
    const double c = hashToUnit(stableHash(seed, x0, y0 + 1, salt));
    const double d = hashToUnit(stableHash(seed, x0 + 1, y0 + 1, salt));

    const double ix0 = lerp(a, b, fade(tx));
    const double ix1 = lerp(c, d, fade(tx));
    return lerp(ix0, ix1, fade(ty));
}

double layeredNoise2D(std::uint64_t seed, double x, double y) {
    const double low = valueNoise2D(seed, x, y, 0.015, 11);
    const double mid = valueNoise2D(seed, x, y, 0.055, 23);
    const double high = valueNoise2D(seed, x, y, 0.16, 37);
    return low * 0.62 + mid * 0.28 + high * 0.10;
}

