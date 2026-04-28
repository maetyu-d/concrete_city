#include "DeltaLayer.hpp"

#include <fstream>
#include <utility>

DeltaLayer::DeltaLayer(std::string path)
    : m_path(std::move(path)) {
}

void DeltaLayer::load() {
    std::ifstream input(m_path);
    if (!input) {
        return;
    }

    TileCoord tile{};
    while (input >> tile.x >> tile.y) {
        m_markers.insert(tile);
    }
}

void DeltaLayer::save() const {
    std::ofstream output(m_path, std::ios::trunc);
    for (const auto& marker : m_markers) {
        output << marker.x << ' ' << marker.y << '\n';
    }
}

void DeltaLayer::toggleMarker(TileCoord tile) {
    auto it = m_markers.find(tile);
    if (it == m_markers.end()) {
        m_markers.insert(tile);
    } else {
        m_markers.erase(it);
    }
    save();
}

bool DeltaLayer::hasMarker(TileCoord tile) const {
    return m_markers.find(tile) != m_markers.end();
}
