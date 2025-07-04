#include "Plate.h"
#include "World.h"
#include "Tile.h"
#include "../ProgressTracker.h"
#include "../Core/WorldGenParameters.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <set>
#include <map>
#include <limits>

namespace WorldGen {
namespace Generators {

std::vector<glm::vec3> GenerateWellDistributedPoints(int numSamples, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> uniform01(0.0f, 1.0f);
    
    std::vector<glm::vec3> points;
    points.reserve(numSamples);
    
    // Use fibonacci sphere for base distribution
    const float goldenAngle = M_PI * (3.0f - sqrt(5.0f));
    
    for (int i = 0; i < numSamples; ++i) {
        // Fibonacci sphere distribution with randomization
        float y = 1.0f - (i / float(numSamples - 1)) * 2.0f;
        float radius = sqrt(1.0f - y * y);
        float theta = goldenAngle * i;
        
        // Add randomness to avoid perfect patterns
        y += (uniform01(rng) - 0.5f) * 0.4f;
        y = glm::clamp(y, -0.98f, 0.98f);
        radius = sqrt(1.0f - y * y);
        theta += (uniform01(rng) - 0.5f) * 0.6f;
        
        glm::vec3 point(
            cos(theta) * radius,
            y,
            sin(theta) * radius
        );
        points.push_back(glm::normalize(point));
    }
    
    return points;
}

std::vector<Plate> GeneratePlates(World* world, int numPlates, uint64_t seed, 
                                 std::shared_ptr<ProgressTracker> progressTracker) {
    if (progressTracker) {
        progressTracker->UpdateProgress(0.0f, "Generating tectonic plates...");
    }
    
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> uniform01(0.0f, 1.0f);
    
    std::vector<Plate> plates;
    
    // All major plates for now (realistic approach)
    int numMajorPlates = numPlates; 
    
    std::cout << "Generating " << numMajorPlates << " major plates..." << std::endl;
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.1f, "Distributing plate centers...");
    }
    
    // Use fast well-distributed points instead of slow Poisson disc
    auto platePositions = GenerateWellDistributedPoints(numPlates, seed);
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.3f, "Creating plate properties...");
    }
    
    // Create major plates with realistic movement patterns
    for (int i = 0; i < numMajorPlates; ++i) {
        Plate plate;
        plate.id = i;
        plate.center = platePositions[i];
        plate.size = PlateSize::Major;
        // Major plates: 30% oceanic (more realistic - most large plates are continental)
        plate.isOceanic = uniform01(rng) < 0.3f;
        
        // Generate more realistic movement patterns
        // Real plates tend to move in coherent patterns, not completely random
        
        // Create a base movement influenced by the plate's position
        glm::vec3 baseMovement;
        
        // Simulate convection-like patterns - plates near equator tend to move faster
        float latitudeInfluence = 1.0f - std::abs(plate.center.y); // Higher at equator
        
        // Create some coherent flow patterns based on position
        float longitude = atan2(plate.center.z, plate.center.x);
        float latitude = asin(plate.center.y);
        
        // Simulate a simplified mantle convection pattern
        // East-west movement influenced by longitude
        float eastWestFlow = sin(longitude * 2.0f + uniform01(rng) * 3.14159f);
        // North-south movement influenced by latitude with some randomness
        float northSouthFlow = cos(latitude * 1.5f + uniform01(rng) * 3.14159f);
        
        // Create movement vector in spherical tangent space
        glm::vec3 east = glm::normalize(glm::cross(plate.center, glm::vec3(0, 1, 0)));
        glm::vec3 north = glm::normalize(glm::cross(east, plate.center));
        
        baseMovement = east * eastWestFlow + north * northSouthFlow;
        
        // Add some randomness but keep the coherent pattern
        glm::vec3 randomComponent = glm::vec3(uniform01(rng)*2.0f-1.0f, uniform01(rng)*2.0f-1.0f, uniform01(rng)*2.0f-1.0f);
        randomComponent = glm::normalize(randomComponent - glm::dot(randomComponent, plate.center) * plate.center);
        
        // Blend coherent movement with random (70% coherent, 30% random)
        plate.movement = glm::normalize(baseMovement * 0.7f + randomComponent * 0.3f);
        
        // Scale movement speed - oceanic plates tend to move faster
        float baseSpeed = plate.isOceanic ? 0.012f : 0.008f;
        plate.movement *= baseSpeed * (0.5f + uniform01(rng) * 0.5f) * latitudeInfluence;
        
        // Rotation rate is generally smaller and somewhat correlated with movement
        plate.rotationRate = (uniform01(rng)*2.0f-1.0f) * 0.0006f * glm::length(plate.movement) * 50.0f;
        
        plates.push_back(plate);
        
        std::cout << "Plate " << i << " (" << (plate.isOceanic ? "oceanic" : "continental") 
                  << "): movement magnitude = " << glm::length(plate.movement) 
                  << ", rotation = " << plate.rotationRate << std::endl;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Plate generation complete");
    }
    
    return plates;
}

void AssignTilesToPlates(World* world, std::vector<Plate>& plates, int targetTotalPlates, uint64_t seed, 
                        std::shared_ptr<ProgressTracker> progressTracker) {
    if (!world || plates.empty()) {
        std::cerr << "Error: Invalid world or no plates to assign" << std::endl;
        return;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.0f, "Assigning tiles to plates...");
    }
    
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> uniform01(0.0f, 1.0f);
    
    const auto& tiles = world->GetTiles();
    std::cout << "Assigning " << tiles.size() << " tiles to " << plates.size() << " plates..." << std::endl;
    
    // Clear existing assignments
    for (auto& plate : plates) {
        plate.tileIds.clear();
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.2f, "Computing plate assignments...");
    }
    
    // Assign all tiles to major plates using distance with noise for natural boundaries
    std::vector<int> tileToPlate(tiles.size(), -1);
    
    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
        const auto& tile = tiles[tileIdx];
        glm::vec3 tileCenter = glm::normalize(tile.GetCenter());
        
        int closestPlate = -1;
        float minDistance = std::numeric_limits<float>::max();
        
        // Find closest plate with noise for natural boundaries
        for (size_t plateIdx = 0; plateIdx < plates.size(); ++plateIdx) {
            float distance = glm::distance(tileCenter, plates[plateIdx].center);
            
            // Add noise for natural boundaries
            float noise = sin(tileCenter.x * 8.0f) * cos(tileCenter.y * 8.0f) * sin(tileCenter.z * 8.0f) * 0.15f;
            float weightedDistance = distance + noise;
            
            if (weightedDistance < minDistance) {
                minDistance = weightedDistance;
                closestPlate = static_cast<int>(plateIdx);
            }
        }
        
        if (closestPlate >= 0) {
            tileToPlate[tileIdx] = closestPlate;
            plates[closestPlate].tileIds.push_back(static_cast<int>(tileIdx));
            
            // Assign plate ID to the tile
            const_cast<Tile&>(tile).SetPlateId(closestPlate);
        }
        
        // Report progress
        if (progressTracker && tileIdx % 1000 == 0) {
            float progress = 0.2f + (static_cast<float>(tileIdx) / tiles.size()) * 0.8f;
            progressTracker->UpdateProgress(progress, "Assigning tiles: " + std::to_string(tileIdx) + "/" + std::to_string(tiles.size()));
        }
    }
    
    // Log assignment results
    for (size_t plateIdx = 0; plateIdx < plates.size(); ++plateIdx) {
        const auto& plate = plates[plateIdx];
        std::cout << "Plate " << plate.id << " (" << (plate.isOceanic ? "oceanic" : "continental") 
                  << "): " << plate.tileIds.size() << " tiles assigned" << std::endl;
    }
    
    // Set base elevations based on plate type
    if (progressTracker) {
        progressTracker->UpdateProgress(0.9f, "Setting base elevations...");
    }
    
    // Apply base elevations with noise for natural terrain
    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
        const auto& tile = tiles[tileIdx];
        int plateId = tile.GetPlateId();
        
        if (plateId >= 0 && plateId < plates.size()) {
            const auto& plate = plates[plateId];
            glm::vec3 tilePos = glm::normalize(tile.GetCenter());
            
            // Base elevation based on plate type (in meters from planet center)
            // Use planet's physical radius as sea level reference
            static const float seaLevelMeters = PlanetParameters().physicalRadiusMeters;
            float baseElevation;
            if (plate.isOceanic) {
                // Oceanic plates: 3000m below sea level (typical ocean depth)
                baseElevation = seaLevelMeters - 3000.0f;
            } else {
                // Continental plates: 500m above sea level (typical continental elevation)
                baseElevation = seaLevelMeters + 500.0f;
            }
            
            // No clamping needed for physical meter values
            
            // Set the elevation
            const_cast<Tile&>(tile).SetElevation(baseElevation);
        }
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Tile assignment complete!");
    }
    
    std::cout << "Tile assignment complete." << std::endl;
}

void SmoothPlateBoundaries(World* world, const std::vector<Plate>& plates,
                          std::shared_ptr<ProgressTracker> progressTracker) {
    if (!world) return;
    
    const auto& tiles = world->GetTiles();
    if (tiles.empty()) return;
    
    std::cout << "Smoothing oceanic-continental plate boundaries..." << std::endl;
    
    const float smoothingDistance = 0.05f; // Distance for smoothing effect
    
    // Find tiles near oceanic-continental boundaries and smooth them
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        int plateId = tile.GetPlateId();
        
        if (plateId >= 0 && plateId < plates.size()) {
            const auto& plate = plates[plateId];
            float currentElevation = tile.GetElevation();
            
            // Look for adjacent tiles with different plate types
            std::vector<float> neighborElevations;
            bool hasOceanicNeighbor = false;
            bool hasContinentalNeighbor = false;
            
            for (int neighborIdx : tile.GetNeighbors()) {
                if (neighborIdx >= 0 && neighborIdx < tiles.size()) {
                    const auto& neighborTile = tiles[neighborIdx];
                    int neighborPlateId = neighborTile.GetPlateId();
                    
                    if (neighborPlateId >= 0 && neighborPlateId < plates.size()) {
                        const auto& neighborPlate = plates[neighborPlateId];
                        
                        // Track different plate types
                        if (neighborPlate.isOceanic != plate.isOceanic) {
                            neighborElevations.push_back(neighborTile.GetElevation());
                            
                            if (neighborPlate.isOceanic) hasOceanicNeighbor = true;
                            else hasContinentalNeighbor = true;
                        }
                    }
                }
            }
            
            // Apply smoothing if this tile is at an oceanic-continental boundary
            if (!neighborElevations.empty() && hasOceanicNeighbor && hasContinentalNeighbor) {
                // Calculate average of neighbor elevations from different plate types
                float avgNeighborElevation = 0.0f;
                for (float elev : neighborElevations) {
                    avgNeighborElevation += elev;
                }
                avgNeighborElevation /= neighborElevations.size();
                
                // Blend current elevation with neighbor average (gentle smoothing)
                float smoothedElevation = currentElevation * 0.7f + avgNeighborElevation * 0.3f;
                
                const_cast<Tile&>(tile).SetElevation(smoothedElevation);
            }
        }
    }
    
    std::cout << "Plate boundary smoothing complete." << std::endl;
}

} // namespace Generators
} // namespace WorldGen