#pragma once

#include <glm/glm.hpp>
#include <unordered_map>

namespace WorldGen {

// Terrain type enumerations
enum class TerrainType {
    Ocean,
    Shallow,
    Beach,
    Lowland,
    Highland,
    Mountain,
    Peak,
    Volcano
};

// Biome type enumerations
enum class BiomeType {
    // Forest biomes
    TropicalRainforest,
    TropicalSeasonalForest,
    TemperateDeciduousForest,
    TemperateRainforest,
    BorealForest,
    MontaneForest,
    
    // Grassland biomes
    TropicalSavanna,
    TemperateGrassland,
    AlpineGrassland,
    
    // Desert and xeric biomes
    HotDesert,
    ColdDesert,
    SemiDesert,
    XericShrubland,
    
    // Tundra and cold biomes
    ArcticTundra,
    AlpineTundra,
    PolarDesert,
    
    // Wetland biomes
    TemperateWetland,
    TropicalWetland,
    
    // Water biomes
    Ocean,
    DeepOcean,
    Reef
};

// Color mapping for terrain types
static const std::unordered_map<TerrainType, glm::vec4> TerrainColors = {
    {TerrainType::Ocean,    {0.0f, 0.2f, 0.5f, 1.0f}},
    {TerrainType::Shallow,  {0.0f, 0.5f, 0.8f, 1.0f}},
    {TerrainType::Beach,    {0.9f, 0.9f, 0.6f, 1.0f}},
    {TerrainType::Lowland,  {0.0f, 0.6f, 0.0f, 1.0f}},
    {TerrainType::Highland, {0.2f, 0.5f, 0.2f, 1.0f}},
    {TerrainType::Mountain, {0.5f, 0.5f, 0.5f, 1.0f}},
    {TerrainType::Peak,     {0.8f, 0.8f, 0.8f, 1.0f}},
    {TerrainType::Volcano,  {0.6f, 0.3f, 0.3f, 1.0f}}
};

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
    TerrainType type;    // Changed from int to TerrainType
    glm::vec4 color;
    
    // Added properties for our new world generation system
    float elevation = 0.5f;     // Normalized elevation [0.0-1.0]
    float humidity = 0.5f;      // Normalized humidity/moisture [0.0-1.0]
    float temperature = 0.5f;   // Normalized temperature [0.0-1.0]
    
    // Reference to the source world tile this was sampled from
    int sourceWorldTileIndex = -1;  // -1 means no source (e.g., procedural generation)
    
    // World position in game coordinates (pixels)
    // This is the final position where this tile should be rendered
    glm::vec2 gamePosition;  // Position in pixels relative to world origin
};

} // namespace WorldGen

// Hash function for the TileCoord struct - defined in the global namespace
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