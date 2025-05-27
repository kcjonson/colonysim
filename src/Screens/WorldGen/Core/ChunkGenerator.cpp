#include "ChunkGenerator.h"
#include "Util.h"
#include "../../../ConfigManager.h"
#include <iostream>
#include <cmath>

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
    
    std::cout << "Generating " << chunkSize << "x" << chunkSize << " chunk (" 
              << (chunkSize * chunkSize) << " tiles)..." << std::endl;
    
    // Generate tiles by sampling the sphere
    int tilesProcessed = 0;
    for (int dy = 0; dy < chunkSize; dy++) {
        if (dy % 10 == 0) {
            std::cout << "  Processing row " << dy << "/" << chunkSize << std::endl;
        }
        for (int dx = 0; dx < chunkSize; dx++) {
            tilesProcessed++;
            // Calculate position relative to chunk center
            // Tiles are arranged with (0,0) at the bottom-left corner
            // So the center is at (chunkSize/2, chunkSize/2)
            float localX = (dx - chunkSize * 0.5f) / tilesPerMeter;
            float localY = (dy - chunkSize * 0.5f) / tilesPerMeter;
            
            // Project this local point onto the sphere
            glm::vec2 localPoint(localX, localY);
            glm::vec3 spherePoint = projectToSphere(localPoint, chunkCenter, chunk->localTangentBasis);
            
            // Find the nearest tile on the sphere
            int nearestIndex = findNearestTile(spherePoint, worldTiles);
            if (nearestIndex < 0 || nearestIndex >= static_cast<int>(worldTiles.size())) {
                continue;
            }
            
            const auto& sourceTile = worldTiles[nearestIndex];
            
            // Create terrain data from the spherical tile
            TerrainData terrainData;
            terrainData.elevation = sourceTile.GetElevation();
            terrainData.humidity = sourceTile.GetMoisture();
            terrainData.temperature = sourceTile.GetTemperature();
            terrainData.type = sourceTile.GetTerrainType();
            
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
            
            // Set color based on terrain type
            auto colorIt = TerrainColors.find(terrainData.type);
            if (colorIt != TerrainColors.end()) {
                terrainData.color = colorIt->second;
            } else {
                terrainData.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
            }
            
            // Store in chunk using local tile coordinates
            chunk->tiles[TileCoord{dx, dy}] = terrainData;
        }
    }
    
    chunk->isLoaded = true;
    chunk->isGenerating = false;
    
    std::cout << "Chunk generation complete! Generated " << tilesProcessed 
              << " tiles with " << chunk->tiles.size() << " stored." << std::endl;
    
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
    float angularX = localPoint.x / PLANET_RADIUS;
    float angularY = localPoint.y / PLANET_RADIUS;
    
    // For small angles, we can use the approximation:
    // The projection of a point (x,y) on the tangent plane is approximately
    // at angular distance sqrt(x² + y²) from the center point
    
    // Create a 3D point on the tangent plane
    glm::vec3 tangentPoint = 
        tangentBasis[0] * localPoint.x +  // East component
        tangentBasis[1] * localPoint.y +  // North component
        tangentBasis[2] * PLANET_RADIUS;   // Up component (at planet radius)
    
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
    const float offsetDistance = angularSize * ChunkGenerator::PLANET_RADIUS;
    
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