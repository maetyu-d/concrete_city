#include "Types.hpp"

#include "Hash.hpp"

std::size_t ChunkCoordHash::operator()(const ChunkCoord& coord) const {
    return static_cast<std::size_t>(stableHash(0xC0FFEEULL, coord.x, coord.y));
}

std::size_t TileCoordHash::operator()(const TileCoord& coord) const {
    return static_cast<std::size_t>(stableHash(0xBADC0DEULL, coord.x, coord.y));
}

const char* tileTypeName(TileType type) {
    switch (type) {
        case TileType::DeepWater: return "deep water";
        case TileType::ShallowWater: return "shallow water";
        case TileType::Grass: return "grass";
        case TileType::Forest: return "forest";
        case TileType::Stone: return "stone";
        case TileType::Strange: return "strange";
    }
    return "unknown";
}

