#include "Chunk.hpp"
#include "WorldGenerator.hpp"

#include <iostream>

int main() {
    WorldGenerator generator(0xC0BA171977ULL);
    const ChunkCoord coord{7, -4};

    const Chunk first(coord, generator);
    const Chunk regenerated(coord, generator);

    for (int y = 0; y < ChunkSize; ++y) {
        for (int x = 0; x < ChunkSize; ++x) {
            if (first.tileAtLocal(x, y) != regenerated.tileAtLocal(x, y)) {
                std::cerr << "Determinism failed at local tile " << x << "," << y << '\n';
                return 1;
            }
        }
    }

    if (first.objects().size() != regenerated.objects().size()) {
        std::cerr << "Determinism failed: object count changed\n";
        return 1;
    }
    for (std::size_t i = 0; i < first.objects().size(); ++i) {
        const auto& a = first.objects()[i];
        const auto& b = regenerated.objects()[i];
        if (a.type != b.type || !(a.tile == b.tile)) {
            std::cerr << "Determinism failed: object " << i << " changed\n";
            return 1;
        }
    }

    std::cout << "Determinism check passed for chunk "
              << coord.x << "," << coord.y
              << " seed " << generator.chunkSeed(coord)
              << " objects " << first.objects().size() << '\n';
    return 0;
}
