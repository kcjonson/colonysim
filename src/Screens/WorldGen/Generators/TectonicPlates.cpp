#include "TectonicPlates.h"
#include "World.h"
#include "Tile.h"
#include "../ProgressTracker.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <queue>
#include <limits>
#include <set>
#include <map>
#include <utility>

namespace WorldGen {
namespace Generators {

// Simplified spherical point distribution (much faster than Poisson disc)
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

std::vector<Plate> GeneratePlates(World* world, int numPlates, uint64_t seed, std::shared_ptr<ProgressTracker> progressTracker) {
    if (progressTracker) {
        progressTracker->UpdateProgress(0.0f, "Generating tectonic plates...");
    }
    
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> uniform01(0.0f, 1.0f);
    
    std::vector<Plate> plates;
    
    // Simple approach: all major plates for now
    int numMajorPlates = numPlates; 
    int numMinorPlates = 0;
    
    std::cout << "Generating " << numMajorPlates << " major plates and " 
              << numMinorPlates << " minor plates (total: " << numPlates << ")..." << std::endl;
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.1f, "Distributing plate centers...");
    }
    
    // Use fast well-distributed points instead of slow Poisson disc
    auto platePositions = GenerateWellDistributedPoints(numPlates, seed);
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.3f, "Creating plate properties...");
    }
    
    // Create major plates first with more realistic movement patterns
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
    
    // Skip minor plate creation - all plates are major plates
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.5f, "Plate generation complete");
    }
    
    return plates;
}

void AssignTilesToPlates(World* world, std::vector<Plate>& plates, int targetTotalPlates, uint64_t seed, std::shared_ptr<ProgressTracker> progressTracker) {
    if (!world || plates.empty()) {
        std::cerr << "Error: Invalid world or no plates to assign" << std::endl;
        return;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.5f, "Assigning tiles to plates...");
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
        progressTracker->UpdateProgress(0.6f, "Computing plate assignments...");
    }
    
    // Realistic plate assignment: Major plates first, then minor plates at boundaries
    std::vector<int> tileToPlate(tiles.size(), -1);
    
    // Phase 1: Assign all tiles to major plates only (like Earth's major plates)
    std::cout << "Phase 1: Assigning all tiles to major plates..." << std::endl;
    
    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
        const auto& tile = tiles[tileIdx];
        glm::vec3 tileCenter = glm::normalize(tile.GetCenter());
        
        int closestMajorPlate = -1;
        float minDistance = std::numeric_limits<float>::max();
        
        // Find closest major plate
        for (size_t plateIdx = 0; plateIdx < plates.size(); ++plateIdx) {
            if (plates[plateIdx].size == PlateSize::Major) {
                float distance = glm::distance(tileCenter, plates[plateIdx].center);
                
                // Add noise for natural boundaries
                float noise = sin(tileCenter.x * 8.0f) * cos(tileCenter.y * 8.0f) * sin(tileCenter.z * 8.0f) * 0.15f;
                float weightedDistance = distance + noise;
                
                if (weightedDistance < minDistance) {
                    minDistance = weightedDistance;
                    closestMajorPlate = static_cast<int>(plateIdx);
                }
            }
        }
        
        if (closestMajorPlate >= 0) {
            tileToPlate[tileIdx] = closestMajorPlate;
            plates[closestMajorPlate].tileIds.push_back(static_cast<int>(tileIdx));
        }
        
        // Report progress
        if (progressTracker && tileIdx % 1000 == 0) {
            float progress = 0.6f + (static_cast<float>(tileIdx) / tiles.size()) * 0.15f;
            progressTracker->UpdateProgress(progress, "Assigning to major plates: " + std::to_string(tileIdx) + "/" + std::to_string(tiles.size()));
        }
    }
    
    // Skip minor plate creation - removed from plate generation loop above - just use major plates
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.9f, "Checking plate sizes...");
    }
    
    // Only ensure very tiny plates get a minimum viable size (no aggressive redistribution)
    size_t avgPlateSize = tiles.size() / plates.size();
    float minViablePercent = 0.125f; // 12.5% of average size
    size_t minViableSize = static_cast<size_t>(avgPlateSize * minViablePercent);
    
    // Only help plates that are extremely tiny (likely due to bad initial placement)
    for (auto& plate : plates) {
        if (plate.tileIds.size() < minViableSize) {
            std::cout << "Plate " << plate.id << " (" << (plate.size == PlateSize::Major ? "major" : "minor") 
                      << ") extremely small: " << plate.tileIds.size() << " tiles, helping to reach " << minViableSize << std::endl;
            
            // Find a moderately large plate to take a few tiles from (not the largest)
            auto donorPlate = std::max_element(plates.begin(), plates.end(),
                [&](const Plate& a, const Plate& b) {
                    // Only consider plates that are reasonably large but not the absolute largest
                    if (a.tileIds.size() < avgPlateSize || b.tileIds.size() < avgPlateSize) {
                        return a.tileIds.size() < b.tileIds.size();
                    }
                    return a.tileIds.size() < b.tileIds.size();
                });
            
            if (donorPlate != plates.end() && donorPlate->tileIds.size() > avgPlateSize) {
                // Transfer only a few tiles to reach minimum viable size
                size_t tilesToTransfer = std::min(minViableSize - plate.tileIds.size(), 
                                                 static_cast<size_t>(5)); // Maximum 5 tiles
                
                // Transfer tiles that are close to the small plate's center
                std::vector<std::pair<float, int>> candidateTiles;
                for (int tileId : donorPlate->tileIds) {
                    float distance = glm::distance(tiles[tileId].GetCenter(), plate.center);
                    candidateTiles.push_back({distance, tileId});
                }
                
                // Sort by distance and take the closest ones
                std::sort(candidateTiles.begin(), candidateTiles.end());
                
                for (size_t i = 0; i < std::min(tilesToTransfer, candidateTiles.size()); ++i) {
                    int tileId = candidateTiles[i].second;
                    
                    // Remove from donor plate
                    auto& donorTiles = donorPlate->tileIds;
                    donorTiles.erase(std::remove(donorTiles.begin(), donorTiles.end(), tileId), 
                                   donorTiles.end());
                    
                    // Add to small plate
                    plate.tileIds.push_back(tileId);
                    tileToPlate[tileId] = plate.id;
                }
                
                std::cout << "Transferred " << tilesToTransfer << " tiles from plate " 
                          << donorPlate->id << " to plate " << plate.id << std::endl;
            }
        }
    }
    
    // Update all tile plate assignments and verify
    std::map<int, int> plateIdCounts;
    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
        int plateId = tileToPlate[tileIdx];
        const_cast<Tile&>(tiles[tileIdx]).SetPlateId(plateId);
        plateIdCounts[plateId]++;
    }
    
    // Debug: Show which plate IDs are actually being used by tiles
    std::cout << "Tile assignment verification:" << std::endl;
    for (const auto& [plateId, count] : plateIdCounts) {
        std::cout << "  Plate ID " << plateId << ": " << count << " tiles assigned" << std::endl;
    }
    
    // Log final plate distribution
    size_t totalTiles = 0;
    
    std::cout << "\n========== FINAL PLATE DISTRIBUTION ==========" << std::endl;
    for (const auto& plate : plates) {
        totalTiles += plate.tileIds.size();
        std::cout << "Plate " << plate.id << ": " << plate.tileIds.size() << " tiles (" 
                  << (plate.isOceanic ? "oceanic" : "continental") << ")" << std::endl;
    }
    std::cout << "Total: " << plates.size() << " plates (" << totalTiles << " tiles)" << std::endl;
    std::cout << "Average plate size: " << (totalTiles / plates.size()) << " tiles" << std::endl;
    std::cout << "============================================\n" << std::endl;
              
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Plate assignment complete!");
    }
}

void GenerateMountains(World* world, const std::vector<Plate>& plates, std::shared_ptr<ProgressTracker> progressTracker) {
    if (!world || plates.empty()) {
        std::cerr << "Error: Invalid world or no plates for mountain generation" << std::endl;
        return;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.0f, "Analyzing plate boundaries...");
    }
    
    const auto& tiles = world->GetTiles();
    std::cout << "Generating mountains for " << tiles.size() << " tiles across " << plates.size() << " plates..." << std::endl;
    
    // Create a map from plate ID to plate for quick lookup
    std::map<int, const Plate*> plateMap;
    for (const auto& plate : plates) {
        plateMap[plate.id] = &plate;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.1f, "Finding plate boundary tiles...");
    }
    
    // Find boundary tiles (tiles with neighbors on different plates)
    std::vector<bool> isBoundaryTile(tiles.size(), false);
    std::vector<std::pair<int, int>> boundaryPairs; // pairs of adjacent plates
    
    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
        const auto& tile = tiles[tileIdx];
        int tileePlateId = tile.GetPlateId();
        
        // Check neighbors for different plate IDs
        const auto& neighbors = tile.GetNeighbors();
        for (int neighborIdx : neighbors) {
            if (neighborIdx >= 0 && neighborIdx < static_cast<int>(tiles.size())) {
                int neighborPlateId = tiles[neighborIdx].GetPlateId();
                if (neighborPlateId != tileePlateId && neighborPlateId >= 0 && tileePlateId >= 0) {
                    isBoundaryTile[tileIdx] = true;
                    isBoundaryTile[neighborIdx] = true;
                    
                    // Record boundary pair (ensure consistent ordering)
                    int plate1 = std::min(tileePlateId, neighborPlateId);
                    int plate2 = std::max(tileePlateId, neighborPlateId);
                    boundaryPairs.push_back({plate1, plate2});
                }
            }
        }
        
        // Report progress
        if (progressTracker && tileIdx % 1000 == 0) {
            float progress = 0.1f + (static_cast<float>(tileIdx) / tiles.size()) * 0.3f;
            progressTracker->UpdateProgress(progress, "Analyzing boundaries: " + std::to_string(tileIdx) + "/" + std::to_string(tiles.size()));
        }
    }
    
    // Remove duplicate boundary pairs
    std::sort(boundaryPairs.begin(), boundaryPairs.end());
    boundaryPairs.erase(std::unique(boundaryPairs.begin(), boundaryPairs.end()), boundaryPairs.end());
    
    std::cout << "Found " << boundaryPairs.size() << " unique plate boundaries" << std::endl;
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.4f, "Calculating boundary interactions...");
    }
    
    // Calculate interaction type and strength for each boundary
    std::map<std::pair<int, int>, float> boundaryStrength;
    std::map<std::pair<int, int>, std::string> boundaryType;
    
    for (const auto& boundary : boundaryPairs) {
        const Plate* plate1 = plateMap[boundary.first];
        const Plate* plate2 = plateMap[boundary.second];
        
        if (!plate1 || !plate2) continue;
        
        // Calculate relative movement at boundary
        glm::vec3 midpoint = glm::normalize((plate1->center + plate2->center) * 0.5f);
        glm::vec3 velocity1 = plate1->movement;
        glm::vec3 velocity2 = plate2->movement;
        glm::vec3 relativeVelocity = velocity2 - velocity1;
        
        // Calculate boundary normal (rough approximation)
        glm::vec3 boundaryDirection = glm::normalize(plate2->center - plate1->center);
        glm::vec3 boundaryNormal = glm::cross(midpoint, boundaryDirection);
        if (glm::length(boundaryNormal) > 0.001f) {
            boundaryNormal = glm::normalize(boundaryNormal);
        } else {
            boundaryNormal = glm::vec3(0, 1, 0); // fallback
        }
        
        // Project relative velocity onto boundary normal
        float convergenceSpeed = glm::dot(relativeVelocity, boundaryNormal);
        float relativeSpeed = glm::length(relativeVelocity);
        
        // Determine boundary type and strength
        float strength = 0.0f;
        std::string type = "transform";
        
        if (std::abs(convergenceSpeed) > relativeSpeed * 0.5f) {
            if (convergenceSpeed > 0) {
                type = "convergent";
                // Mountain strength depends on plate types and convergence speed
                strength = std::abs(convergenceSpeed) * 1000.0f; // Scale factor
                
                // Continental-continental collision creates highest mountains
                if (!plate1->isOceanic && !plate2->isOceanic) {
                    strength *= 2.0f;
                } else if (plate1->isOceanic != plate2->isOceanic) {
                    // Oceanic-continental subduction creates medium mountains
                    strength *= 1.5f;
                }
            } else {
                type = "divergent";
                strength = std::abs(convergenceSpeed) * 500.0f; // Rifting creates valleys/lower elevation
            }
        } else {
            type = "transform";
            strength = relativeSpeed * 200.0f; // Transform faults create moderate relief
        }
        
        boundaryStrength[boundary] = strength;
        boundaryType[boundary] = type;
        
        std::cout << "Boundary " << boundary.first << "-" << boundary.second 
                  << ": " << type << " (strength: " << strength << ")" << std::endl;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.6f, "Applying mountain formation...");
    }
    
    // Apply elevation changes to boundary tiles
    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
        if (!isBoundaryTile[tileIdx]) continue;
        
        const auto& tile = tiles[tileIdx];
        int tilePlateId = tile.GetPlateId();
        float currentElevation = tile.GetElevation();
        float elevationChange = 0.0f;
        
        // Find which boundaries this tile participates in
        const auto& neighbors = tile.GetNeighbors();
        for (int neighborIdx : neighbors) {
            if (neighborIdx >= 0 && neighborIdx < static_cast<int>(tiles.size())) {
                int neighborPlateId = tiles[neighborIdx].GetPlateId();
                if (neighborPlateId != tilePlateId && neighborPlateId >= 0 && tilePlateId >= 0) {
                    
                    // Get boundary info
                    int plate1 = std::min(tilePlateId, neighborPlateId);
                    int plate2 = std::max(tilePlateId, neighborPlateId);
                    std::pair<int, int> boundaryKey = {plate1, plate2};
                    
                    auto strengthIt = boundaryStrength.find(boundaryKey);
                    auto typeIt = boundaryType.find(boundaryKey);
                    
                    if (strengthIt != boundaryStrength.end() && typeIt != boundaryType.end()) {
                        float strength = strengthIt->second;
                        const std::string& type = typeIt->second;
                        
                        // Apply elevation change based on boundary type
                        if (type == "convergent") {
                            elevationChange += strength * 0.001f; // Mountains
                        } else if (type == "divergent") {
                            elevationChange -= strength * 0.0005f; // Rifts/valleys
                        } else if (type == "transform") {
                            // Transform boundaries create moderate relief variation
                            float noise = sin(tile.GetCenter().x * 10.0f) * cos(tile.GetCenter().z * 10.0f);
                            elevationChange += strength * 0.0003f * noise;
                        }
                    }
                }
            }
        }
        
        // Apply the elevation change
        if (std::abs(elevationChange) > 0.001f) {
            float newElevation = currentElevation + elevationChange;
            newElevation = glm::clamp(newElevation, 0.0f, 1.0f);
            const_cast<Tile&>(tile).SetElevation(newElevation);
            
            // Update terrain type based on new elevation
            const float waterLevel = 0.4f;
            TerrainType newTerrainType;
            if (newElevation < waterLevel - 0.2f) {
                newTerrainType = TerrainType::Ocean;
            } else if (newElevation < waterLevel - 0.05f) {
                newTerrainType = TerrainType::Shallow;
            } else if (newElevation < waterLevel + 0.05f) {
                newTerrainType = TerrainType::Beach;
            } else if (newElevation < waterLevel + 0.3f) {
                newTerrainType = TerrainType::Lowland;
            } else if (newElevation < waterLevel + 0.6f) {
                newTerrainType = TerrainType::Highland;
            } else if (newElevation < waterLevel + 0.8f) {
                newTerrainType = TerrainType::Mountain;
            } else {
                newTerrainType = TerrainType::Peak;
            }
            const_cast<Tile&>(tile).SetTerrainType(newTerrainType);
        }
        
        // Report progress
        if (progressTracker && tileIdx % 1000 == 0) {
            float progress = 0.6f + (static_cast<float>(tileIdx) / tiles.size()) * 0.4f;
            progressTracker->UpdateProgress(progress, "Applying mountains: " + std::to_string(tileIdx) + "/" + std::to_string(tiles.size()));
        }
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Mountain generation complete!");
    }
    
    std::cout << "Mountain generation complete." << std::endl;
}


} // namespace Generators
} // namespace WorldGen