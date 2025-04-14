#pragma once

#include <unordered_map>
#include <utility>
#include <string>
#include <glm/glm.hpp>

// Hash function for std::pair (since it's not provided by default)
namespace std {
    template <>
    struct hash<std::pair<int, int>> {
        size_t operator()(const std::pair<int, int>& p) const {
            return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
        }
    };
}

namespace WorldGen {

struct TerrainData {
    float height;
    float resource;
    int type;
    glm::vec4 color;
};

class TerrainGenerator {
public:
    // Static methods for terrain generation
    static float noise2D(float x, float y, unsigned int seed);
    static float fbm(float x, float y, int octaves, float persistence, unsigned int seed);
    static unsigned int getHashedSeed(const std::string& seed);
    
    // Main terrain generation method
    static void generateTerrain(
        std::unordered_map<std::pair<int, int>, TerrainData>& terrainData,
        int generateDistance,
        unsigned int hashedSeed
    );

private:
    static constexpr float PI = 3.14159265358979323846f;
    
    // Private constructor to prevent instantiation
    TerrainGenerator() = default;
};

} // namespace WorldGen