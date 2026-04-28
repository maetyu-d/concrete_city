#include "Chunk.hpp"

Chunk::Chunk(ChunkCoord coord, const WorldGenerator& generator)
    : m_coord(coord) {
    for (int y = 0; y < ChunkSize; ++y) {
        for (int x = 0; x < ChunkSize; ++x) {
            const TileCoord worldTile{
                coord.x * ChunkSize + x,
                coord.y * ChunkSize + y
            };
            m_tiles[static_cast<std::size_t>(y * ChunkSize + x)] = generator.tileAt(worldTile);
        }
    }
    m_objects = generator.objectsForChunk(coord);
}

TileType Chunk::tileAtLocal(int localX, int localY) const {
    return m_tiles[static_cast<std::size_t>(localY * ChunkSize + localX)];
}
