#pragma once

#include "Chunk.hpp"
#include "Object.hpp"
#include "Types.hpp"
#include "WorldGenerator.hpp"

#include <raylib.h>
#include <memory>
#include <unordered_map>

class DeltaLayer;

class ChunkManager {
public:
    explicit ChunkManager(const WorldGenerator& generator);

    void updateAround(TileCoord playerTile);
    TileType tileAt(TileCoord worldTile);
    std::vector<WorldObject> objectsNear(TileCoord center, int radiusChunks);
    Color objectColor(ObjectType type) const;
    Color tileColor(TileType type, TileCoord tile) const;
    ChunkCoord chunkCoordForTile(TileCoord worldTile) const;
    int loadedChunkCount() const { return static_cast<int>(m_chunks.size()); }

private:
    const Chunk& getOrCreate(ChunkCoord coord);

    const WorldGenerator& m_generator;
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> m_chunks;
    int m_loadRadius = 2;
};
