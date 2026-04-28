#pragma once

#include "Types.hpp"
#include "WorldGenerator.hpp"

#include <array>
#include <vector>

class Chunk {
public:
    Chunk(ChunkCoord coord, const WorldGenerator& generator);

    ChunkCoord coord() const { return m_coord; }
    TileType tileAtLocal(int localX, int localY) const;
    const std::vector<WorldObject>& objects() const { return m_objects; }

private:
    ChunkCoord m_coord;
    std::array<TileType, ChunkSize * ChunkSize> m_tiles{};
    std::vector<WorldObject> m_objects;
};
