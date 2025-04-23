#pragma once

#include <unordered_map>
#include <utility>
#include <string>
#include <glm/glm.hpp>
#include <functional> // Include for std::hash

namespace WorldGen {

// Use a dedicated struct for coordinates instead of std::pair
struct TileCoord {
    int x;
    int y;

    bool operator==(const TileCoord& other) const {
        return x == other.x && y == other.y;
    }
};

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

    // Main terrain generation method - updated to use TileCoord
    static void generateTerrain(
        std::unordered_map<TileCoord, TerrainData>& terrainData, // Use TileCoord
        int generateDistance,
        unsigned int hashedSeed
    );

private:
    static constexpr float PI = 3.14159265358979323846f;

    // Private constructor to prevent instantiation
    TerrainGenerator() = default;
};

} // namespace WorldGen

// Hash function for the TileCoord struct - defined in the global namespace
// but specializing std::hash
namespace std {
    template <>
    struct hash<WorldGen::TileCoord> {
        size_t operator()(const WorldGen::TileCoord& coord) const noexcept {
            size_t h1 = std::hash<int>()(coord.x);
            size_t h2 = std::hash<int>()(coord.y);
            // Use Boost::hash_combine style combination for potentially better distribution
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
        }
    };
}