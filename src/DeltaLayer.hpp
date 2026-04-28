#pragma once

#include "Types.hpp"

#include <string>
#include <unordered_set>

class DeltaLayer {
public:
    explicit DeltaLayer(std::string path);

    void load();
    void save() const;
    void toggleMarker(TileCoord tile);
    bool hasMarker(TileCoord tile) const;
    const std::unordered_set<TileCoord, TileCoordHash>& markers() const { return m_markers; }

private:
    std::string m_path;
    std::unordered_set<TileCoord, TileCoordHash> m_markers;
};

