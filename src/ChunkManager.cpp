#include "ChunkManager.hpp"

#include "Hash.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
int floorDiv(int value, int divisor) {
    int result = value / divisor;
    const int remainder = value % divisor;
    if (remainder != 0 && ((remainder < 0) != (divisor < 0))) {
        --result;
    }
    return result;
}

int floorMod(int value, int divisor) {
    const int mod = value % divisor;
    return mod < 0 ? mod + divisor : mod;
}

Color adjustColor(Color color, int amount) {
    const auto clamp = [](int value) {
        return static_cast<unsigned char>(std::clamp(value, 0, 255));
    };
    return {
        clamp(static_cast<int>(color.r) + amount),
        clamp(static_cast<int>(color.g) + amount),
        clamp(static_cast<int>(color.b) + amount),
        color.a
    };
}
}

ChunkManager::ChunkManager(const WorldGenerator& generator)
    : m_generator(generator) {
}

void ChunkManager::updateAround(TileCoord playerTile) {
    const ChunkCoord center = chunkCoordForTile(playerTile);
    for (int y = center.y - m_loadRadius; y <= center.y + m_loadRadius; ++y) {
        for (int x = center.x - m_loadRadius; x <= center.x + m_loadRadius; ++x) {
            getOrCreate({x, y});
        }
    }

    std::vector<ChunkCoord> unload;
    for (const auto& [coord, chunk] : m_chunks) {
        const int dx = std::abs(coord.x - center.x);
        const int dy = std::abs(coord.y - center.y);
        if (dx > m_loadRadius + 1 || dy > m_loadRadius + 1) {
            unload.push_back(coord);
        }
    }
    for (const auto& coord : unload) {
        m_chunks.erase(coord);
    }
}

TileType ChunkManager::tileAt(TileCoord worldTile) {
    const ChunkCoord chunkCoord = chunkCoordForTile(worldTile);
    const Chunk& chunk = getOrCreate(chunkCoord);
    return chunk.tileAtLocal(floorMod(worldTile.x, ChunkSize), floorMod(worldTile.y, ChunkSize));
}

std::vector<WorldObject> ChunkManager::objectsNear(TileCoord center, int radiusChunks) {
    std::vector<WorldObject> objects;
    const ChunkCoord centerChunk = chunkCoordForTile(center);
    for (int y = centerChunk.y - radiusChunks; y <= centerChunk.y + radiusChunks; ++y) {
        for (int x = centerChunk.x - radiusChunks; x <= centerChunk.x + radiusChunks; ++x) {
            const Chunk& chunk = getOrCreate({x, y});
            const auto& chunkObjects = chunk.objects();
            objects.insert(objects.end(), chunkObjects.begin(), chunkObjects.end());
        }
    }
    return objects;
}

ChunkCoord ChunkManager::chunkCoordForTile(TileCoord worldTile) const {
    return {floorDiv(worldTile.x, ChunkSize), floorDiv(worldTile.y, ChunkSize)};
}

const Chunk& ChunkManager::getOrCreate(ChunkCoord coord) {
    auto it = m_chunks.find(coord);
    if (it != m_chunks.end()) {
        return *it->second;
    }

    auto chunk = std::make_unique<Chunk>(coord, m_generator);
    const auto [inserted, _] = m_chunks.emplace(coord, std::move(chunk));
    return *inserted->second;
}

Color ChunkManager::objectColor(ObjectType type) const {
    switch (type) {
        case ObjectType::Tree: return {39, 63, 48, 255};
        case ObjectType::Stone: return {126, 132, 126, 255};
        case ObjectType::Ruin: return {112, 116, 110, 255};
        case ObjectType::Marker: return {176, 36, 31, 255};
        case ObjectType::Anomaly: return {197, 62, 176, 255};
    }
    return WHITE;
}

Color ChunkManager::tileColor(TileType type, TileCoord tile) const {
    const float distance = std::sqrt(static_cast<float>(tile.x * tile.x + tile.y * tile.y));
    const auto shift = static_cast<int>(std::min(distance / 90.0f, 42.0f));
    const int variation = static_cast<int>(stableHash(0xA7A105ULL, tile.x, tile.y, 0xC0102) % 13) - 6;

    Color color = BLACK;
    switch (type) {
        case TileType::DeepWater: color = {10, 29, static_cast<unsigned char>(42 + shift / 2), 255}; break;
        case TileType::ShallowWater: color = {28, 60, static_cast<unsigned char>(63 + shift / 2), 255}; break;
        case TileType::Grass: color = {58, static_cast<unsigned char>(71 + shift / 5), 55, 255}; break;
        case TileType::Forest: color = {28, static_cast<unsigned char>(50 + shift / 5), 38, 255}; break;
        case TileType::Stone: color = {static_cast<unsigned char>(108 + shift / 5), static_cast<unsigned char>(112 + shift / 5), static_cast<unsigned char>(106 + shift / 4), 255}; break;
        case TileType::Strange: color = {static_cast<unsigned char>(132 + shift / 2), 63, static_cast<unsigned char>(124 + shift / 3), 255}; break;
    }
    return adjustColor(color, variation);
}
