#pragma once

#include <cstdint>

std::uint64_t hashCombine(std::uint64_t a, std::uint64_t b);
std::uint64_t stableHash(std::uint64_t seed, std::int64_t x, std::int64_t y, std::uint64_t salt = 0);
double hashToUnit(std::uint64_t hash);
double valueNoise2D(std::uint64_t seed, double x, double y, double frequency, std::uint64_t salt);
double layeredNoise2D(std::uint64_t seed, double x, double y);

