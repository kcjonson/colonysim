#include "ChunkGenerator.h"
#include "Util.h"
#include "WorldGenParameters.h"
#include "../../../ConfigManager.h"
#include <iostream>
#include <cmath>
#include <chrono>
#include <iomanip>

namespace WorldGen {
namespace Core {

std::unique_ptr<ChunkData> ChunkGenerator::generateChunk(
    const WorldGen::Generators::World& worldGenerator,
    const glm::vec3& chunkCenter
) {
    auto chunk = std::make_unique<ChunkData>();
    chunk->coord = ChunkCoord(chunkCenter);
    
    // Get configuration
    const auto& config = ConfigManager::getInstance();
    const int chunkSize = config.getChunkSize();
    const float tilesPerMeter = config.getTilesPerMeter();
    const int tileSampleRate = config.getTileSampleRate();
    
    // Create local tangent basis for this chunk
    chunk->localTangentBasis = createLocalTangentBasis(chunkCenter);
    
    // Get world tiles for sampling
    const auto& worldTiles = worldGenerator.GetTiles();
    if (worldTiles.empty()) {
        std::cerr << "ERROR: No world tiles available for sampling!" << std::endl;
        return chunk;
    }
    
    // Calculate the size of the chunk in meters
    const float chunkSizeMeters = chunkSize / tilesPerMeter;
    
    // Get current time for logging
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    // Convert chunk center to world coordinates for logging
    PlanetParameters planetParams;
    float theta = std::atan2(chunkCenter.z, chunkCenter.x);
    float phi = std::asin(glm::clamp(chunkCenter.y, -1.0f, 1.0f));
    glm::vec2 chunkWorldPos(theta * planetParams.physicalRadiusMeters, phi * planetParams.physicalRadiusMeters);
    
    std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
              << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
              << "Generating " << chunkSize << "x" << chunkSize << " chunk at world pos ("
              << static_cast<int>(chunkWorldPos.x) << ", " << static_cast<int>(chunkWorldPos.y) << ")" << std::endl;
    
    // OPTIMIZATION: Track the current world tile as we sample to avoid repeated searches
    // Since we sample in a spatial pattern (left-to-right, top-to-bottom), 
    // adjacent samples are likely in the same world tile or immediate neighbors
    int currentWorldTileIndex = -1;
    
    // Helper lambda to sample a single tile
    auto sampleTile = [&](int dx, int dy) -> TerrainData {
        float localX = (dx - chunkSize * 0.5f) / tilesPerMeter;
        float localY = (dy - chunkSize * 0.5f) / tilesPerMeter;
        
        glm::vec2 localPoint(localX, localY);
        glm::vec3 spherePoint = projectToSphere(localPoint, chunkCenter, chunk->localTangentBasis);
        
        // Use optimized local search starting from the previous tile
        // This reduces search from O(n) where n = total world tiles to O(k) where k ≈ 6-12 neighbors
        currentWorldTileIndex = worldGenerator.FindTileContainingPoint(spherePoint, currentWorldTileIndex);
        
        if (currentWorldTileIndex < 0 || currentWorldTileIndex >= static_cast<int>(worldTiles.size())) {
            // Return default ocean tile
            TerrainData defaultData;
            defaultData.type = TerrainType::Ocean;
            defaultData.height = 0.0f;
            defaultData.resource = 0.0f;
            defaultData.sourceWorldTileIndex = -1;
            return defaultData;
        }
        
        const auto& sourceTile = worldTiles[currentWorldTileIndex];
        
        TerrainData terrainData;
        terrainData.elevation = sourceTile.GetElevation();
        terrainData.humidity = sourceTile.GetMoisture();
        terrainData.temperature = sourceTile.GetTemperature();
        terrainData.type = sourceTile.GetTerrainType();
        terrainData.sourceWorldTileIndex = currentWorldTileIndex;  // Store the reference
        
        // Set height based on terrain type
        if (terrainData.type == TerrainType::Ocean || 
            terrainData.type == TerrainType::Shallow) {
            terrainData.height = 0.0f + (0.1f * terrainData.elevation);
        } else {
            terrainData.height = 0.2f + (0.8f * terrainData.elevation);
        }
        
        // Calculate resource value based on biome type
        float resourceMultiplier = 0.5f;
        switch (sourceTile.GetBiomeType()) {
            case BiomeType::TropicalRainforest:
            case BiomeType::TemperateRainforest:
            case BiomeType::BorealForest:
                resourceMultiplier = 1.0f;
                break;
            case BiomeType::TemperateGrassland:
            case BiomeType::TropicalSavanna:
                resourceMultiplier = 0.7f;
                break;
            case BiomeType::HotDesert:
            case BiomeType::ColdDesert:
                resourceMultiplier = 0.2f;
                break;
            default:
                resourceMultiplier = 0.5f;
        }
        terrainData.resource = resourceMultiplier * terrainData.humidity;
        
        // COORDINATE SYSTEM: Calculate final game position for this tile
        // This eliminates the need for complex coordinate transformation in World class
        // See docs/ChunkedWorldImplementation.md for complete coordinate system documentation
        
        // Step 1: Convert sphere position to world coordinates (meters from origin)
        glm::vec2 worldPos = sphereToWorld(spherePoint);
        
        // Step 2: Convert world coordinates to game coordinates (pixels)
        terrainData.gamePosition = worldToGame(worldPos);
        
        return terrainData;
    };
    
    int tilesProcessed = 0;
    int samplesPerformed = 0;
    
    // OPTIMIZATION: Sample perimeter first to detect homogeneous chunks
    // If all perimeter samples map to the same world tile, we can fill the interior
    // without sampling every game tile (massive speedup for ocean/desert/forest chunks)
    std::vector<std::pair<TileCoord, TerrainData>> perimeterSamples;
    bool allSameWorldTile = true;
    int firstWorldTileIndex = -1;
    bool firstSample = true;
    
    // Sample corners first (always sampled)
    std::vector<TileCoord> corners = {
        {0, 0}, {chunkSize-1, 0}, {chunkSize-1, chunkSize-1}, {0, chunkSize-1}
    };
    
    for (const auto& corner : corners) {
        TerrainData data = sampleTile(corner.x, corner.y);
        samplesPerformed++;
        perimeterSamples.push_back({corner, data});
        
        if (firstSample) {
            firstWorldTileIndex = data.sourceWorldTileIndex;
            firstSample = false;
        } else if (data.sourceWorldTileIndex != firstWorldTileIndex) {
            allSameWorldTile = false;
        }
    }
    
    // Sample edges (with sampling rate)
    // Top edge
    for (int x = tileSampleRate; x < chunkSize - 1; x += tileSampleRate) {
        TerrainData data = sampleTile(x, 0);
        samplesPerformed++;
        perimeterSamples.push_back({{x, 0}, data});
        if (data.sourceWorldTileIndex != firstWorldTileIndex) allSameWorldTile = false;
    }
    
    // Right edge
    for (int y = tileSampleRate; y < chunkSize - 1; y += tileSampleRate) {
        TerrainData data = sampleTile(chunkSize - 1, y);
        samplesPerformed++;
        perimeterSamples.push_back({{chunkSize - 1, y}, data});
        if (data.sourceWorldTileIndex != firstWorldTileIndex) allSameWorldTile = false;
    }
    
    // Bottom edge
    for (int x = chunkSize - 1 - tileSampleRate; x > 0; x -= tileSampleRate) {
        TerrainData data = sampleTile(x, chunkSize - 1);
        samplesPerformed++;
        perimeterSamples.push_back({{x, chunkSize - 1}, data});
        if (data.sourceWorldTileIndex != firstWorldTileIndex) allSameWorldTile = false;
    }
    
    // Left edge
    for (int y = chunkSize - 1 - tileSampleRate; y > 0; y -= tileSampleRate) {
        TerrainData data = sampleTile(0, y);
        samplesPerformed++;
        perimeterSamples.push_back({{0, y}, data});
        if (data.sourceWorldTileIndex != firstWorldTileIndex) allSameWorldTile = false;
    }
    
    // Store perimeter samples
    for (const auto& [coord, data] : perimeterSamples) {
        chunk->tiles[coord] = data;
        tilesProcessed++;
    }
    
    if (allSameWorldTile && !perimeterSamples.empty()) {
        // Chunk is homogeneous - fill interior with the same terrain
        std::cout << "  Homogeneous chunk detected (world tile: " << firstWorldTileIndex 
                  << "), filling interior..." << std::endl;
        
        // Use the first sample as template (they're all the same type anyway)
        TerrainData templateData = perimeterSamples[0].second;
        
        // Fill interior
        for (int dy = 1; dy < chunkSize - 1; dy++) {
            for (int dx = 1; dx < chunkSize - 1; dx++) {
                // Skip if already sampled (shouldn't happen with current logic)
                if (chunk->tiles.count({dx, dy}) == 0) {
                    chunk->tiles[{dx, dy}] = templateData;
                    tilesProcessed++;
                }
            }
        }
        
        std::cout << "  Optimized generation: " << samplesPerformed << " samples for " 
                  << tilesProcessed << " tiles" << std::endl;
    } else {
        // Chunk is heterogeneous - need to sample interior
        std::cout << "  Heterogeneous chunk detected, sampling interior..." << std::endl;
        
        // Sample interior tiles
        for (int dy = 1; dy < chunkSize - 1; dy++) {
            for (int dx = 1; dx < chunkSize - 1; dx++) {
                // Skip if already sampled
                if (chunk->tiles.count({dx, dy}) == 0) {
                    TerrainData data = sampleTile(dx, dy);
                    samplesPerformed++;
                    chunk->tiles[{dx, dy}] = data;
                    tilesProcessed++;
                }
            }
        }
        
        std::cout << "  Full sampling: " << samplesPerformed << " samples for " 
                  << tilesProcessed << " tiles" << std::endl;
    }
    
    chunk->isLoaded = true;
    chunk->isGenerating = false;
    
    // Log completion with timestamp
    auto endTime = std::chrono::system_clock::now();
    auto endTime_t = std::chrono::system_clock::to_time_t(endTime);
    auto endMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime.time_since_epoch()) % 1000;
    
    std::cout << "[" << std::put_time(std::localtime(&endTime_t), "%H:%M:%S") 
              << "." << std::setfill('0') << std::setw(3) << endMs.count() << "] "
              << "Chunk complete at (" << static_cast<int>(chunkWorldPos.x) << ", " 
              << static_cast<int>(chunkWorldPos.y) << ") - " 
              << tilesProcessed << " tiles, " << samplesPerformed << " samples" << std::endl;
    
    return chunk;
}

glm::mat3 ChunkGenerator::createLocalTangentBasis(const glm::vec3& pointOnSphere) {
    // Ensure the point is normalized
    glm::vec3 up = glm::normalize(pointOnSphere);
    
    // Choose a reference vector for "north"
    // We use the global Y-axis (north pole) unless we're too close to it
    glm::vec3 northPole(0, 1, 0);
    
    // If we're near the poles, use a different reference
    if (std::abs(glm::dot(up, northPole)) > 0.99f) {
        northPole = glm::vec3(1, 0, 0);  // Use X-axis instead
    }
    
    // Calculate east (tangent to longitude lines)
    glm::vec3 east = glm::normalize(glm::cross(northPole, up));
    
    // Calculate north (tangent to latitude lines)
    glm::vec3 north = glm::normalize(glm::cross(up, east));
    
    // Return matrix with columns [east, north, up]
    return glm::mat3(east, north, up);
}

glm::vec3 ChunkGenerator::projectToSphere(
    const glm::vec2& localPoint,
    const glm::vec3& chunkCenter,
    const glm::mat3& tangentBasis
) {
    // PROJECTION METHOD:
    // We use a gnomonic projection (perspective from sphere center).
    // This minimizes distortion for small areas and preserves straight lines.
    
    // Convert local meters to angular displacement
    PlanetParameters planetParams;
    float angularX = localPoint.x / planetParams.physicalRadiusMeters;
    float angularY = localPoint.y / planetParams.physicalRadiusMeters;
    
    // For small angles, we can use the approximation:
    // The projection of a point (x,y) on the tangent plane is approximately
    // at angular distance sqrt(x² + y²) from the center point
    
    // Create a 3D point on the tangent plane
    glm::vec3 tangentPoint = 
        tangentBasis[0] * localPoint.x +  // East component
        tangentBasis[1] * localPoint.y +  // North component
        tangentBasis[2] * planetParams.physicalRadiusMeters;   // Up component (at planet radius)
    
    // Project back onto the unit sphere
    // This accounts for the curvature of the Earth
    glm::vec3 spherePoint = glm::normalize(tangentPoint);
    
    return spherePoint;
}

// Implement the helper function for getting neighboring chunk centers
std::vector<glm::vec3> getNeighboringChunkCenters(const glm::vec3& center, float angularSize) {
    std::vector<glm::vec3> neighbors;
    neighbors.reserve(8);
    
    // Create local tangent basis at this center
    glm::mat3 basis = ChunkGenerator::createLocalTangentBasis(center);
    
    // Generate 8 neighbors in a grid pattern
    // Using angular size to determine offset distance
    PlanetParameters planetParams;
    const float offsetDistance = angularSize * planetParams.physicalRadiusMeters;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;  // Skip center
            
            glm::vec2 offset(dx * offsetDistance, dy * offsetDistance);
            glm::vec3 neighborCenter = ChunkGenerator::projectToSphere(offset, center, basis);
            neighbors.push_back(neighborCenter);
        }
    }
    
    return neighbors;
}

} // namespace Core
} // namespace WorldGen