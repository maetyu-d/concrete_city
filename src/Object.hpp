#pragma once

#include "Types.hpp"

enum class ObjectType {
    Tree,
    Stone,
    Ruin,
    Marker,
    Anomaly
};

struct WorldObject {
    ObjectType type = ObjectType::Tree;
    TileCoord tile{};
};

const char* objectTypeName(ObjectType type);

