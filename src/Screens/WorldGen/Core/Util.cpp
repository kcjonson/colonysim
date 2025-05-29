#include "Util.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/gtx/norm.hpp>
#include "../../Game/Tile.h"
#include "../../Game/World.h"
#include "TerrainTypes.h"
#include "ChunkGenerator.h"
#include "WorldGenParameters.h"
#include "../../../ConfigManager.h"

// Define M_PI if not defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define a float version of PI for our calculations
constexpr float M_PIf = 3.14159265358979323846f;

namespace WorldGen {
namespace Core {

/*
// DEPRECATED: This function is unused and deprecated. 
// It was part of the old world generation system that has been replaced by chunked generation.
// Helper function to convert 3D spherical coordinates to 2D tile coordinates
TileCoord sphericalToTileCoord(const glm::vec3& spherePoint, float sampleRate) {
    // Convert from 3D sphere point to 2D map coordinates using an equirectangular projection
    float longitude = std::atan2(spherePoint.z, spherePoint.x);
    float latitude = std::asin(std::max(-1.0f, std::min(1.0f, spherePoint.y))); // Clamp the input to avoid NaN
    
    // Map from [-π, π] × [-π/2, π/2] to grid coordinates
    float mapWidth = sampleRate * 2.0f * M_PIf;
    float mapHeight = sampleRate * M_PIf;
    
    // Apply a hard limit to dimensions to prevent issues with very large sample rates
    const float MAX_DIMENSION = 1024.0f;
    if (mapWidth > MAX_DIMENSION) {
        float scaleFactor = MAX_DIMENSION / mapWidth;
        mapWidth = MAX_DIMENSION;
        mapHeight *= scaleFactor;
    }
    if (mapHeight > MAX_DIMENSION) {
        float scaleFactor = MAX_DIMENSION / mapHeight;
        mapHeight = MAX_DIMENSION;
        mapWidth *= scaleFactor;
    }
    
    // Use double for the intermediate calculation to avoid precision issues
    double normX = (longitude + M_PIf) / (2.0 * M_PIf);
    double normY = (latitude + M_PIf / 2.0) / M_PIf;
    
    // Keep these values in the [0,1] range to avoid edge cases
    normX = std::max(0.0, std::min(1.0, normX));
    normY = std::max(0.0, std::min(1.0, normY));
    
    int x = static_cast<int>(normX * mapWidth);
    int y = static_cast<int>(normY * mapHeight);
    
    // Ensure coordinates are within bounds
    x = std::max(0, std::min(x, static_cast<int>(mapWidth) - 1));
    y = std::max(0, std::min(y, static_cast<int>(mapHeight) - 1));
    
    return {x, y};
}
*/

// Public helper function to find the nearest tile in the generator world to a given point
int findNearestTile(const glm::vec3& point, const std::vector<WorldGen::Generators::Tile>& tiles) {
    // Check for empty tiles vector to avoid crashes
    if (tiles.empty()) {
        std::cerr << "ERROR: Empty tiles vector in findNearestTile" << std::endl;
        return -1;
    }
    
    int nearestIndex = 0;
    float minDistance = std::numeric_limits<float>::max();
    
    // Check if the point is valid (not NaN or Inf)
    if (std::isnan(point.x) || std::isnan(point.y) || std::isnan(point.z) ||
        std::isinf(point.x) || std::isinf(point.y) || std::isinf(point.z)) {
        std::cerr << "WARNING: Invalid point in findNearestTile: (" 
                  << point.x << ", " << point.y << ", " << point.z << ")" << std::endl;
        return 0; // Return the first tile as a fallback
    }
    
    // Size check to use signed comparison
    int numTiles = static_cast<int>(tiles.size());
    for (int i = 0; i < numTiles; i++) {
        float distance = glm::distance2(point, tiles[i].GetCenter());
        if (distance < minDistance) {
            minDistance = distance;
            nearestIndex = i;
        }
    }
    
    return nearestIndex;
}

/*
// ============================================================================
// DEPRECATED WORLD GENERATION CODE
// ============================================================================
// 
// This entire block is deprecated and unused. It was part of the original
// world generation system that created a single large 2D map of the entire
// sphere. This has been replaced by the chunked generation system which:
// - Generates small chunks on-demand
// - Uses less memory
// - Supports infinite world exploration
// - Has better performance
//
// This code is kept commented out for reference but should not be used.
// ============================================================================
std::unique_ptr<World> createGameWorld(
    const WorldGen::Generators::World& worldGenerator,
    GameState& gameState,
    float sampleRate,
    Camera* camera,
    GLFWwindow* window,
    const std::string& seed
) {
    // Validate input parameters
    if (sampleRate <= 0.0f) {
        std::cerr << "WARNING: Invalid sample rate: " << sampleRate << ", using default (16.0)" << std::endl;
        sampleRate = 16.0f;
    }
    
    // Cap sample rate to a reasonable maximum to prevent memory issues
    const float MAX_SAMPLE_RATE = 48.0f;
    if (sampleRate > MAX_SAMPLE_RATE) {
        std::cerr << "WARNING: Sample rate " << sampleRate << " exceeds maximum (" 
                  << MAX_SAMPLE_RATE << "), clamping." << std::endl;
        sampleRate = MAX_SAMPLE_RATE;
    }
    
    try {
        // Create a new Game::World
        std::unique_ptr<World> gameWorld = std::make_unique<World>(gameState, seed, camera, window);
        
        // Get generator tiles
        const std::vector<WorldGen::Generators::Tile>& generatorTiles = worldGenerator.GetTiles();
        
        // Safety check for empty tiles
        if (generatorTiles.empty()) {
            std::cerr << "ERROR: Generator world contains no tiles!" << std::endl;
            return nullptr;
        }
    
    // Map to store terrain data for game world
    std::unordered_map<TileCoord, TerrainData> terrainDataMap;
      // Calculate the dimensions of the equirectangular projection
    // Add a hard limit on the maximum dimensions to prevent memory issues
    const float MAX_DIMENSION = 1024.0f; // Maximum dimension in either direction
    
    float mapWidth = sampleRate * 2.0f * M_PIf;
    float mapHeight = sampleRate * M_PIf;
    
    // Apply maximum size constraint
    if (mapWidth > MAX_DIMENSION) {
        float scaleFactor = MAX_DIMENSION / mapWidth;
        mapWidth = MAX_DIMENSION;
        mapHeight *= scaleFactor;
    }
    if (mapHeight > MAX_DIMENSION) {
        float scaleFactor = MAX_DIMENSION / mapHeight;
        mapHeight = MAX_DIMENSION;
        mapWidth *= scaleFactor;
    }
    
    std::cout << "Creating game world with dimensions: " << 
                 static_cast<int>(mapWidth) << "x" << static_cast<int>(mapHeight) << 
                 " tiles (sample rate: " << sampleRate << ")" << std::endl;
    std::cout << "Source world has " << generatorTiles.size() << " tiles" << std::endl;    // Sample the sphere surface to generate square game tiles
    int tilesProcessed = 0;
    int maxTilesToProcess = 1048576; // Set a reasonable maximum number of tiles (1024*1024)
    
    // Add additional parameters to have a chunked approach
    const int MAX_TILES_PER_CHUNK = 1000;
    const int MAX_CHUNKS = 30; // Limit the number of chunks to process
    
    try {
        std::cout << "Starting to generate game tiles..." << std::endl;
        
        // Process the tiles in chunks to avoid overwhelming memory
        for (int chunk = 0; chunk < MAX_CHUNKS; chunk++) {
            int startY = (chunk * MAX_TILES_PER_CHUNK) / static_cast<int>(mapWidth);
            int endY = std::min(static_cast<int>(mapHeight), ((chunk + 1) * MAX_TILES_PER_CHUNK) / static_cast<int>(mapWidth) + 1);
            
            if (startY >= static_cast<int>(mapHeight)) {
                break; // We've processed all rows
            }
            
            std::cout << "Processing chunk " << chunk + 1 << " of up to " << MAX_CHUNKS 
                      << " (rows " << startY << " to " << endY << ")" << std::endl;
            
            for (int y = startY; y < endY && y < static_cast<int>(mapHeight); y++) {
                for (int x = 0; x < static_cast<int>(mapWidth); x++) {
                    // Safety check to prevent processing too many tiles
                    if (++tilesProcessed > maxTilesToProcess) {
                        std::cerr << "WARNING: Maximum number of tiles exceeded. Limiting world size." << std::endl;
                        break;
                    }
                
                // Convert from 2D map coordinates back to 3D sphere point
                float longitude = (static_cast<float>(x) / mapWidth) * 2.0f * M_PIf - M_PIf;
                float latitude = (static_cast<float>(y) / mapHeight) * M_PIf - M_PIf / 2.0f;
                
                float cosLat = std::cos(latitude);
                glm::vec3 spherePoint(
                    std::cos(longitude) * cosLat,
                    std::sin(latitude),
                    std::sin(longitude) * cosLat
                );
            
            // Normalize to ensure it's on the unit sphere
            spherePoint = glm::normalize(spherePoint);
              // Find the nearest generator tile
            int nearestTileIndex = findNearestTile(spherePoint, generatorTiles);
            
            // Add error checking for invalid index
            if (nearestTileIndex < 0 || nearestTileIndex >= static_cast<int>(generatorTiles.size())) {
                std::cerr << "WARNING: Invalid nearestTileIndex: " << nearestTileIndex 
                          << " (valid range: 0-" << generatorTiles.size() - 1 << ")" << std::endl;
                          
                // Use a fallback terrain data if we can't find a valid tile
                TerrainData fallbackData;
                fallbackData.type = TerrainType::Ocean;
                fallbackData.color = glm::vec4(0.0f, 0.2f, 0.5f, 1.0f);
                fallbackData.height = 0.0f;
                fallbackData.resource = 0.0f;
                
                terrainDataMap[{x, y}] = fallbackData;
                continue;
            }
            
            const auto& nearestTile = generatorTiles[nearestTileIndex];
            
            // Create terrain data for this game tile based on the generator tile
            TerrainData terrainData;
            terrainData.elevation = nearestTile.GetElevation();            
            // TerrainData uses humidity while Tile uses moisture for the same concept
            terrainData.humidity = nearestTile.GetMoisture();
            terrainData.temperature = nearestTile.GetTemperature();
            terrainData.type = nearestTile.GetTerrainType();
            
            // Set height based on terrain type (ocean is lower than land)
            if (terrainData.type == TerrainType::Ocean || terrainData.type == TerrainType::Shallow) {
                terrainData.height = 0.0f + (0.1f * terrainData.elevation); // Ocean/shallow water height
            } else {
                terrainData.height = 0.2f + (0.8f * terrainData.elevation); // Land height
            }
            
            // Set resource amount based on terrain and biome
            // This is a simple example - could be made more complex based on biome and other factors
            float resourceMultiplier = 0.0f;
            switch (nearestTile.GetBiomeType()) {
                case BiomeType::TropicalRainforest:
                case BiomeType::TemperateRainforest:
                case BiomeType::BorealForest:
                    resourceMultiplier = 1.0f; // High resources in forests
                    break;
                case BiomeType::TemperateGrassland:
                case BiomeType::TropicalSavanna:
                    resourceMultiplier = 0.7f; // Medium resources in grasslands
                    break;
                case BiomeType::HotDesert:
                case BiomeType::ColdDesert:
                    resourceMultiplier = 0.2f; // Low resources in deserts
                    break;
                default:
                    resourceMultiplier = 0.5f; // Default for other biomes
            }
            terrainData.resource = resourceMultiplier * terrainData.humidity;
            
            // Set color based on terrain type
            auto colorIter = TerrainColors.find(terrainData.type);
            if (colorIter != TerrainColors.end()) {
                terrainData.color = colorIter->second;
            } else {
                terrainData.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f); // Default gray
            }                    // Store terrain data in map
                    terrainDataMap[{x, y}] = terrainData;
                }
                
                // If we hit the tile limit, break out of all loops
                if (tilesProcessed > maxTilesToProcess) {
                    break;
                }
            }
            
            // Break out of chunk loop if we hit the tile limit
            if (tilesProcessed > maxTilesToProcess) {
                break;
            }
            
            // After each chunk, report progress
            std::cout << "Completed chunk " << chunk + 1 << ", generated " << terrainDataMap.size() 
                      << " tiles so far" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during world generation: " << e.what() << std::endl;
        // Continue with what we have so far, rather than failing completely
    }
      // Set terrain data to game world
    if (terrainDataMap.empty()) {
        std::cerr << "ERROR: No terrain data was generated!" << std::endl;
        return nullptr;
    }
    
    std::cout << "Generated " << terrainDataMap.size() << " terrain tiles" << std::endl;
    
    try {
        gameWorld->setTerrainData(terrainDataMap);
        
        // Initialize the game world
        if (!gameWorld->initialize()) {
            std::cerr << "Failed to initialize game world!" << std::endl;
            return nullptr;
        }
        
        return gameWorld;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during game world setup: " << e.what() << std::endl;
        return nullptr;
    }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during game world creation: " << e.what() << std::endl;
        return nullptr;
    }
}
*/

std::unique_ptr<ChunkData> generateInitialChunk(
    const WorldGen::Generators::World& worldGenerator,
    const glm::vec3& landingLocation
) {
    // Simply delegate to the ChunkGenerator
    // The initial chunk is centered at the landing location
    return ChunkGenerator::generateChunk(worldGenerator, landingLocation);
}

glm::vec2 sphereToLatLong(const glm::vec3& spherePos) {
    /**
     * COORDINATE TRANSFORMATION FOUNDATION
     * 
     * ⚠️  CRITICAL: This function is the cornerstone of all coordinate conversions.
     *     See docs/ChunkedWorldImplementation.md for complete system documentation.
     *     Every coordinate transformation should use this for consistency.
     * 
     * This function is the cornerstone of all coordinate conversions in the system.
     * Every other coordinate transformation should use this function to ensure consistency.
     * 
     * MATHEMATICAL DETAILS:
     * - Input: 3D Cartesian coordinates on unit sphere (x,y,z where x²+y²+z² = 1)
     * - Output: Spherical coordinates (longitude, latitude) in radians
     * 
     * CONVERSION FORMULAS:
     * - Longitude = atan2(z, x)    [Range: -π to +π]
     * - Latitude = asin(y)         [Range: -π/2 to +π/2]
     * 
     * COORDINATE CONVENTIONS:
     * - Longitude: East is positive, West is negative
     * - Latitude: North is positive, South is negative
     * - Prime meridian (0° longitude) = sphere position (1,0,0)
     * - Equator (0° latitude) = y = 0
     * - North pole (+90° latitude) = sphere position (0,1,0)
     */
    
    // Calculate longitude using atan2 for proper quadrant handling
    // atan2(z,x) gives angle from positive X-axis toward positive Z-axis
    float longitude = std::atan2(spherePos.z, spherePos.x);
    
    // Calculate latitude using asin, with clamping to handle floating-point precision issues
    // asin(y) gives angle from XZ-plane toward positive Y-axis (north)
    float latitude = std::asin(glm::clamp(spherePos.y, -1.0f, 1.0f));
    
    return glm::vec2(longitude, latitude);
}

glm::vec2 sphereToWorld(const glm::vec3& spherePos) {
    /**
     * WORLD COORDINATE SYSTEM CONVERSION
     * 
     * Converts from 3D sphere position to 2D world coordinates in meters.
     * This is the intermediate coordinate system used for chunk positioning.
     * 
     * COORDINATE SYSTEM DEFINITION:
     * - Origin (0,0) = Prime meridian and Equator = sphere position (1,0,0)
     * - X-axis = Longitude in meters (positive = East, negative = West)
     * - Y-axis = Latitude in meters (positive = North, negative = South)
     * 
     * CONVERSION PROCESS:
     * 1. Extract longitude/latitude in radians using shared function
     * 2. Convert angular measurements to linear distance using planet radius
     * 3. Formula: distance = angle_in_radians * planet_radius_in_meters
     */
    
    // Step 1: Get longitude and latitude in radians
    glm::vec2 latLong = sphereToLatLong(spherePos);
    float longitude = latLong.x;
    float latitude = latLong.y;
    
    // Step 2: Convert to world coordinates using planet radius
    PlanetParameters planetParams; // Use default Earth parameters
    glm::vec2 worldPos;
    worldPos.x = longitude * planetParams.physicalRadiusMeters;  // Longitude -> X (east/west)
    worldPos.y = latitude * planetParams.physicalRadiusMeters;   // Latitude -> Y (north/south)
    
    return worldPos;
}

glm::vec2 worldToGame(const glm::vec2& worldPos) {
    /**
     * GAME COORDINATE SYSTEM CONVERSION
     * 
     * Converts from world coordinates (meters) to game coordinates (pixels).
     * This is the final coordinate system used for tile rendering and camera positioning.
     * 
     * COORDINATE SYSTEM PROPERTIES:
     * - Units: Pixels
     * - Origin: Same as world origin (prime meridian/equator)
     * - Scale: Determined by tile size and tile density configurations
     * 
     * CONVERSION FORMULA:
     * pixels = meters * (tiles_per_meter * pixels_per_tile)
     * 
     * DEFAULT CONFIGURATION:
     * - Tile size: 10 pixels per tile
     * - Tile density: 1.0 tiles per meter
     * - Result: 1 meter = 10 pixels
     */
    
    // Get configuration values
    const auto& config = ConfigManager::getInstance();
    const float tileSize = config.getTileSize();           // pixels per tile (default: 10)
    const float tilesPerMeter = config.getTilesPerMeter(); // tiles per meter (default: 1.0)
    
    // Calculate meters to pixels conversion factor
    const float metersToPixels = tilesPerMeter * tileSize;  // 1.0 * 10 = 10 pixels per meter
    
    return worldPos * metersToPixels;
}

} // namespace Core
} // namespace WorldGen
