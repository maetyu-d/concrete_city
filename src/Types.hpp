#pragma once

#include <cstdint>
#include <cstddef>

constexpr int ChunkSize = 64;
constexpr int TileSize = 12;

struct ChunkCoord {
    int x = 0;
    int y = 0;

    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y;
    }
};

struct TileCoord {
    int x = 0;
    int y = 0;

    bool operator==(const TileCoord& other) const {
        return x == other.x && y == other.y;
    }
};

struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& coord) const;
};

struct TileCoordHash {
    std::size_t operator()(const TileCoord& coord) const;
};

enum class TileType : std::uint8_t {
    DeepWater,
    ShallowWater,
    Grass,
    Forest,
    Stone,
    Strange
};

const char* tileTypeName(TileType type);
