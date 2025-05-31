#include "ContinentalMargin.h"
#include "World.h"
#include "Plate.h"
#include "Tile.h"
#include "../ProgressTracker.h"
#include <glm/geometric.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace WorldGen {
namespace Generators {

void CreateRealisticContinentalMargins(World* world, const std::vector<Plate>& plates, 
                                     const ContinentalMarginParams& params, uint64_t seed,
                                     std::shared_ptr<ProgressTracker> progressTracker) {
    if (!world) return;
    
    std::cout << "Creating realistic continental margins..." << std::endl;
    
    const auto& tiles = world->GetTiles();
    if (tiles.empty()) return;
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.0f, "Applying isostatic adjustment...");
    }
    
    // Phase 1: Create wide-area transitions at oceanic-continental boundaries
    // Keep the original plate elevations but smooth over a larger distance
    
    const float transitionDistance = 0.6f; // Distance for transition zone (hundreds of tiles)
    
    // Find distance to nearest oceanic-continental boundary for each tile
    std::vector<float> distanceToBoundary(tiles.size(), 999.0f); // Start with very high distance
    std::vector<float> targetElevation(tiles.size(), 0.0f);
    
    // First pass: identify boundary tiles and calculate target elevations
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        int plateId = tile.GetPlateId();
        
        if (plateId >= 0 && plateId < plates.size()) {
            const auto& plate = plates[plateId];
            
            // Check if this tile is near an oceanic-continental boundary
            bool nearOceanicContinentalBoundary = false;
            
            for (int neighborIdx : tile.GetNeighbors()) {
                if (neighborIdx >= 0 && neighborIdx < tiles.size()) {
                    const auto& neighborTile = tiles[neighborIdx];
                    int neighborPlateId = neighborTile.GetPlateId();
                    
                    if (neighborPlateId >= 0 && neighborPlateId < plates.size()) {
                        const auto& neighborPlate = plates[neighborPlateId];
                        
                        // Check for oceanic-continental boundary
                        if (neighborPlate.isOceanic != plate.isOceanic) {
                            nearOceanicContinentalBoundary = true;
                            break;
                        }
                    }
                }
            }
            
            if (nearOceanicContinentalBoundary) {
                distanceToBoundary[i] = 0.0f; // This tile is at boundary
                
                // Calculate realistic transition target based on plate type
                float currentElevation = tile.GetElevation();
                if (plate.isOceanic) {
                    // Oceanic tiles: transition upward toward continental shelf
                    targetElevation[i] = currentElevation + 0.08f; // Stronger raise ocean floor near continents
                } else {
                    // Continental tiles: transition downward toward shelf depth  
                    targetElevation[i] = currentElevation - 0.1f;  // Stronger lower continental edges to create shelf
                }
            }
        }
    }
    
    // Second pass: propagate boundary effects outward in waves (limited)
    const int maxWaves = 10; // Limit propagation to 10 waves
    
    for (int wave = 0; wave < maxWaves; ++wave) {
        std::vector<bool> affected(tiles.size(), false);
        
        for (size_t i = 0; i < tiles.size(); ++i) {
            if (distanceToBoundary[i] == wave) { // This tile is at current wave distance
                const auto& tile = tiles[i];
                
                // Propagate to all neighbors
                for (int neighborIdx : tile.GetNeighbors()) {
                    if (neighborIdx >= 0 && neighborIdx < tiles.size()) {
                        if (distanceToBoundary[neighborIdx] > wave + 1) {
                            distanceToBoundary[neighborIdx] = wave + 1;
                            targetElevation[neighborIdx] = targetElevation[i];
                            affected[neighborIdx] = true;
                        }
                    }
                }
            }
        }
        
        // If no tiles were affected, we're done
        bool anyAffected = false;
        for (bool a : affected) {
            if (a) { anyAffected = true; break; }
        }
        if (!anyAffected) break;
    }
    
    // Third pass: apply gradual elevation transitions
    int tilesAffected = 0;
    const int maxTransitionWaves = 5; // Affect boundary tiles + 5 waves outward
    
    for (size_t i = 0; i < tiles.size(); ++i) {
        if (distanceToBoundary[i] <= maxTransitionWaves) {
            tilesAffected++;
            const auto& tile = tiles[i];
            float currentElevation = tile.GetElevation();
            
            // Calculate blend factor based on wave number (1.0 at boundary, 0.0 at max waves)
            float blendFactor = 1.0f - (distanceToBoundary[i] / (float)maxTransitionWaves);
            blendFactor = glm::clamp(blendFactor, 0.0f, 1.0f);
            
            // Apply smooth exponential transition (more gradual)
            blendFactor = 1.0f - exp(-2.0f * blendFactor); // Exponential curve for smoother transition
            
            // Blend current elevation with target elevation
            float smoothedElevation = currentElevation * (1.0f - blendFactor) + targetElevation[i] * blendFactor;
            const_cast<Tile&>(tile).SetElevation(glm::clamp(smoothedElevation, 0.0f, 1.0f));
        }
    }
    
    std::cout << "Continental margin smoothing affected " << tilesAffected << " tiles out of " << tiles.size() << std::endl;
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.3f, "Forming passive margin continental shelves...");
    }
    
    // Phase 2: Form continental shelves for passive margins
    FormPassiveMarginShelves(world, plates, params, seed);
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.7f, "Creating active margin subduction features...");
    }
    
    // Phase 3: Create subduction zone features for active margins  
    FormActiveMarginTrenches(world, plates, params, seed + 1);
    
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Continental margin formation complete!");
    }
    
    std::cout << "Continental margin formation complete." << std::endl;
}

float CalculateIsostaticElevation(float crustThickness, float crustDensity, 
                                const ContinentalMarginParams& params) {
    // Airy isostatic model: h = (ρ_mantle - ρ_crust) * thickness / ρ_mantle
    // But we need to normalize this to our 0-1 elevation system where 0.4 = sea level
    
    float densityContrast = params.mantleDensity - crustDensity;
    float buoyancy = densityContrast / params.mantleDensity;
    
    // Calculate raw isostatic height in km
    float isostaticHeight = buoyancy * crustThickness;
    
    // For reference:
    // - Oceanic crust (7km, 3.0 g/cm³): height = (3.3-3.0)/3.3 * 7 = 0.636 km above mantle
    // - Continental crust (35km, 2.7 g/cm³): height = (3.3-2.7)/3.3 * 35 = 6.36 km above mantle
    // - Difference: ~5.7 km, which should map to oceanic vs continental elevation difference
    
    // Map to our normalized elevation system:
    // - Oceanic baseline (0.636 km) -> 0.2 (Ocean depth)
    // - Continental average (6.36 km) -> 0.6 (Land elevation) 
    // - Total range of ~6 km maps to elevation range of 0.4
    
    float normalizedElevation = (isostaticHeight - 0.636f) / 6.0f * 0.4f + 0.2f;
    
    return glm::clamp(normalizedElevation, 0.0f, 1.0f);
}

MarginType DetermineMarginType(World* world, const std::vector<Plate>& plates, int tileIndex) {
    const auto& tiles = world->GetTiles();
    if (tileIndex < 0 || tileIndex >= tiles.size()) return MarginType::Passive;
    
    const auto& tile = tiles[tileIndex];
    int plateId = tile.GetPlateId();
    
    if (plateId < 0 || plateId >= plates.size()) return MarginType::Passive;
    
    const auto& plate = plates[plateId];
    
    // Check if this tile is near a plate boundary with convergent motion
    const auto& neighbors = tile.GetNeighbors();
    
    for (int neighborIdx : neighbors) {
        if (neighborIdx >= 0 && neighborIdx < tiles.size()) {
            const auto& neighborTile = tiles[neighborIdx];
            int neighborPlateId = neighborTile.GetPlateId();
            
            if (neighborPlateId >= 0 && neighborPlateId < plates.size() && 
                neighborPlateId != plateId) {
                
                const auto& neighborPlate = plates[neighborPlateId];
                
                // Check for oceanic-continental convergence (subduction)
                if ((plate.isOceanic && !neighborPlate.isOceanic) ||
                    (!plate.isOceanic && neighborPlate.isOceanic)) {
                    
                    // Calculate relative motion to detect convergence
                    glm::vec3 relativeMotion = plate.movement - neighborPlate.movement;
                    glm::vec3 boundaryNormal = glm::normalize(tile.GetCenter() - neighborTile.GetCenter());
                    
                    // If plates are moving toward each other, it's an active margin
                    float convergence = glm::dot(relativeMotion, boundaryNormal);
                    if (convergence > 0.1f) { // Threshold for significant convergence
                        return MarginType::Active;
                    }
                }
            }
        }
    }
    
    return MarginType::Passive;
}

void FormPassiveMarginShelves(World* world, const std::vector<Plate>& plates,
                            const ContinentalMarginParams& params, uint64_t seed) {
    const auto& tiles = world->GetTiles();
    
    // Find ocean-continent boundaries for shelf formation
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        int plateId = tile.GetPlateId();
        
        if (plateId >= 0 && plateId < plates.size()) {
            const auto& plate = plates[plateId];
            
            // Only process continental tiles near oceanic boundaries
            if (!plate.isOceanic && DetermineMarginType(world, plates, i) == MarginType::Passive) {
                
                // Calculate distance to nearest oceanic plate
                float minDistanceToOcean = 1.0f;
                for (const auto& neighborIdx : tile.GetNeighbors()) {
                    if (neighborIdx >= 0 && neighborIdx < tiles.size()) {
                        const auto& neighborTile = tiles[neighborIdx];
                        int neighborPlateId = neighborTile.GetPlateId();
                        
                        if (neighborPlateId >= 0 && neighborPlateId < plates.size() &&
                            plates[neighborPlateId].isOceanic) {
                            
                            float distance = glm::distance(tile.GetCenter(), neighborTile.GetCenter());
                            minDistanceToOcean = std::min(minDistanceToOcean, distance);
                        }
                    }
                }
                
                // Apply continental shelf profile if near ocean
                if (minDistanceToOcean < params.maxShelfWidth) {
                    float shelfFactor = minDistanceToOcean / params.maxShelfWidth;
                    
                    // Create gradual slope from land to shelf break
                    float currentElevation = tile.GetElevation();
                    float shelfElevation = 0.4f - params.shelfBreakDepth * (1.0f - shelfFactor);
                    
                    // Blend between current elevation and shelf profile
                    float blendFactor = 1.0f - shelfFactor;
                    float newElevation = currentElevation * shelfFactor + shelfElevation * blendFactor;
                    
                    // Add thermal subsidence for cooling lithosphere
                    glm::vec3 pos = tile.GetCenter();
                    float thermalAge = sin(pos.x * 5.0f + pos.y * 7.0f) * 0.5f + 0.5f;
                    newElevation -= params.thermalSubsidenceRate * thermalAge;
                    
                    // Add sediment loading subsidence
                    float sedimentThickness = params.sedimentationRate * (1.0f - shelfFactor);
                    newElevation -= sedimentThickness * params.sedimentLoadingFactor;
                    
                    const_cast<Tile&>(tile).SetElevation(glm::clamp(newElevation, 0.0f, 1.0f));
                }
            }
        }
    }
}

void FormActiveMarginTrenches(World* world, const std::vector<Plate>& plates,
                            const ContinentalMarginParams& params, uint64_t seed) {
    const auto& tiles = world->GetTiles();
    
    // Create subduction trenches and associated features
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        int plateId = tile.GetPlateId();
        
        if (plateId >= 0 && plateId < plates.size()) {
            const auto& plate = plates[plateId];
            
            if (DetermineMarginType(world, plates, i) == MarginType::Active) {
                
                // Find distance to active plate boundary
                float minDistanceToConvergentBoundary = 1.0f;
                bool isSubductingPlate = false;
                
                for (const auto& neighborIdx : tile.GetNeighbors()) {
                    if (neighborIdx >= 0 && neighborIdx < tiles.size()) {
                        const auto& neighborTile = tiles[neighborIdx];
                        int neighborPlateId = neighborTile.GetPlateId();
                        
                        if (neighborPlateId >= 0 && neighborPlateId < plates.size() &&
                            neighborPlateId != plateId) {
                            
                            const auto& neighborPlate = plates[neighborPlateId];
                            
                            // Check for oceanic plate subducting under continental
                            if (plate.isOceanic && !neighborPlate.isOceanic) {
                                isSubductingPlate = true;
                                float distance = glm::distance(tile.GetCenter(), neighborTile.GetCenter());
                                minDistanceToConvergentBoundary = std::min(minDistanceToConvergentBoundary, distance);
                            }
                        }
                    }
                }
                
                // Apply subduction zone features
                if (minDistanceToConvergentBoundary < params.forearcBasinWidth) {
                    float distanceFactor = minDistanceToConvergentBoundary / params.forearcBasinWidth;
                    
                    if (isSubductingPlate) {
                        // Create deep oceanic trench
                        float trenchDepth = params.trenchDepth * (1.0f - distanceFactor);
                        float currentElevation = tile.GetElevation();
                        float newElevation = currentElevation - trenchDepth;
                        
                        const_cast<Tile&>(tile).SetElevation(glm::clamp(newElevation, 0.0f, 1.0f));
                    } else {
                        // Continental side: create forearc basin and potential uplift
                        float currentElevation = tile.GetElevation();
                        
                        // Close to trench: forearc basin (slight depression)
                        if (distanceFactor < 0.3f) {
                            float basinDepression = 0.05f * (1.0f - distanceFactor / 0.3f);
                            currentElevation -= basinDepression;
                        }
                        // Further inland: accretionary wedge uplift
                        else if (distanceFactor < 0.7f) {
                            float uplift = 0.1f * ((distanceFactor - 0.3f) / 0.4f);
                            currentElevation += uplift;
                        }
                        
                        const_cast<Tile&>(tile).SetElevation(glm::clamp(currentElevation, 0.0f, 1.0f));
                    }
                }
            }
        }
    }
}

} // namespace Generators
} // namespace WorldGen