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
    
    // Create major plates first
    for (int i = 0; i < numMajorPlates; ++i) {
        Plate plate;
        plate.id = i;
        plate.center = platePositions[i];
        plate.size = PlateSize::Major;
        // Major plates: 50% oceanic (more balanced)
        plate.isOceanic = uniform01(rng) < 0.5f;
        // Moderate movement for major plates
        glm::vec3 randomDir = glm::vec3(uniform01(rng)*2.0f-1.0f, uniform01(rng)*2.0f-1.0f, uniform01(rng)*2.0f-1.0f);
        plate.movement = glm::normalize(randomDir - glm::dot(randomDir, plate.center) * plate.center);
        plate.movement *= uniform01(rng) * 0.008f;
        plate.rotationRate = (uniform01(rng)*2.0f-1.0f) * 0.0008f;
        plates.push_back(plate);
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


} // namespace Generators
} // namespace WorldGen