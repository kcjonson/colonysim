#pragma once

#include <unordered_map>
#include <string>
#include <functional>
#include "TerrainTypes.h"

namespace WorldGen {

class TerrainGenerator {
public:
    // Static methods for terrain generation
    static float noise2D(float x, float y, unsigned int seed);
    static float fbm(float x, float y, int octaves, float persistence, unsigned int seed);
    static unsigned int getHashedSeed(const std::string& seed);

    // Main terrain generation method
    static void generateTerrain(
        std::unordered_map<TileCoord, TerrainData>& terrainData,
        int generateDistance,
        unsigned int hashedSeed
    );

private:
    static constexpr float PI = 3.14159265358979323846f;

    // Private constructor to prevent instantiation
    TerrainGenerator() = default;
};

} // namespace WorldGen