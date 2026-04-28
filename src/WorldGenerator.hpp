#pragma once

#include "Object.hpp"
#include "Types.hpp"

#include <cstdint>
#include <vector>

class WorldGenerator {
public:
    explicit WorldGenerator(std::uint64_t worldSeed);

    TileType tileAt(TileCoord worldTile) const;
    std::vector<WorldObject> objectsForChunk(ChunkCoord chunk) const;
    std::uint64_t chunkSeed(ChunkCoord chunk) const;
    std::uint64_t worldSeed() const { return m_worldSeed; }

private:
    std::uint64_t m_worldSeed;
};
