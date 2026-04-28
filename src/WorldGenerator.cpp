#include "WorldGenerator.hpp"

#include "Hash.hpp"

#include <algorithm>
#include <cmath>

namespace {
bool canPlaceObjectOn(TileType terrain) {
    return terrain != TileType::DeepWater && terrain != TileType::ShallowWater;
}
}

WorldGenerator::WorldGenerator(std::uint64_t worldSeed)
    : m_worldSeed(worldSeed) {
}

std::uint64_t WorldGenerator::chunkSeed(ChunkCoord chunk) const {
    return stableHash(m_worldSeed, chunk.x, chunk.y, 0x5EED);
}

TileType WorldGenerator::tileAt(TileCoord worldTile) const {
    // Core procedural rule: terrain is derived only from world seed and absolute tile position.
    // No generated tile map is saved, so revisiting this coordinate reproduces the same answer.
    const double x = static_cast<double>(worldTile.x);
    const double y = static_cast<double>(worldTile.y);
    const double biome = layeredNoise2D(m_worldSeed, x, y);
    const double detail = valueNoise2D(m_worldSeed, x, y, 0.22, 91);
    const double distance = std::sqrt(x * x + y * y);
    const double weirdness = std::min(distance / 4200.0, 1.0);
    const double anomaly = valueNoise2D(m_worldSeed, x, y, 0.08, 1337);

    if (anomaly > 0.985 - weirdness * 0.035) {
        return TileType::Strange;
    }
    if (biome < 0.25) {
        return TileType::DeepWater;
    }
    if (biome < 0.34) {
        return TileType::ShallowWater;
    }
    if (biome > 0.73 - weirdness * 0.08 || detail > 0.82) {
        return TileType::Stone;
    }
    if (biome > 0.57 - weirdness * 0.05 || detail > 0.64) {
        return TileType::Forest;
    }
    return TileType::Grass;
}

std::vector<WorldObject> WorldGenerator::objectsForChunk(ChunkCoord chunk) const {
    std::vector<WorldObject> objects;
    const std::uint64_t seed = chunkSeed(chunk);

    for (int i = 0; i < 34; ++i) {
        const auto localX = static_cast<int>(stableHash(seed, i, 1, 0xA11CE) % ChunkSize);
        const auto localY = static_cast<int>(stableHash(seed, i, 2, 0xA11CE) % ChunkSize);
        const TileCoord tile{chunk.x * ChunkSize + localX, chunk.y * ChunkSize + localY};
        const TileType terrain = tileAt(tile);
        if (!canPlaceObjectOn(terrain)) {
            continue;
        }

        bool tooClose = false;
        for (const auto& existing : objects) {
            const int dx = existing.tile.x - tile.x;
            const int dy = existing.tile.y - tile.y;
            if (dx * dx + dy * dy < 64) {
                tooClose = true;
                break;
            }
        }
        if (tooClose) {
            continue;
        }

        const double roll = hashToUnit(stableHash(seed, localX, localY, 0x0B1EC7));
        const double distance = std::sqrt(static_cast<double>(tile.x * tile.x + tile.y * tile.y));
        const double weirdness = std::min(distance / 4200.0, 1.0);

        if (terrain == TileType::Strange && roll > 0.60) {
            objects.push_back({ObjectType::Anomaly, tile});
        } else if (terrain == TileType::Stone && roll > 0.70) {
            objects.push_back({ObjectType::Stone, tile});
        } else if (roll > 0.945 - weirdness * 0.065) {
            objects.push_back({ObjectType::Ruin, tile});
        } else if (roll > 0.972 - weirdness * 0.018) {
            objects.push_back({ObjectType::Marker, tile});
        } else if (terrain == TileType::Forest && roll > 0.88 + weirdness * 0.08) {
            objects.push_back({ObjectType::Tree, tile});
        } else if (terrain == TileType::Grass && roll > 0.985) {
            objects.push_back({ObjectType::Tree, tile});
        }
    }

    return objects;
}
