#include "Mountain.h"
#include "World.h"
#include "Tile.h"
#include "../ProgressTracker.h"
#include "../Core/TerrainTypes.h"
#include "../Core/WorldGenParameters.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <map>
#include <set>
#include <limits>

namespace WorldGen {
namespace Generators {

std::vector<BoundaryInfo> AnalyzePlateBoundaries(World* world, const std::vector<Plate>& plates) {
    if (!world || plates.empty()) {
        return {};
    }
    
    const auto& tiles = world->GetTiles();
    std::vector<BoundaryInfo> boundaries;
    std::set<std::pair<int, int>> processedBoundaries;
    
    // Create plate lookup map
    std::map<int, const Plate*> plateMap;
    for (const auto& plate : plates) {
        plateMap[plate.id] = &plate;
    }
    
    std::cout << "Analyzing boundaries between " << plates.size() << " plates..." << std::endl;
    
    // Find boundary segments by examining adjacent tiles on different plates
    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
        const auto& tile = tiles[tileIdx];
        int tilePlateId = tile.GetPlateId();
        
        if (tilePlateId < 0) continue; // Skip unassigned tiles
        
        const auto& neighbors = tile.GetNeighbors();
        for (int neighborIdx : neighbors) {
            if (neighborIdx >= 0 && neighborIdx < static_cast<int>(tiles.size())) {
                int neighborPlateId = tiles[neighborIdx].GetPlateId();
                
                if (neighborPlateId != tilePlateId && neighborPlateId >= 0) {
                    // Found a boundary between different plates
                    int plate1 = std::min(tilePlateId, neighborPlateId);
                    int plate2 = std::max(tilePlateId, neighborPlateId);
                    std::pair<int, int> boundaryKey = {plate1, plate2};
                    
                    // Process each boundary pair only once
                    if (processedBoundaries.find(boundaryKey) == processedBoundaries.end()) {
                        processedBoundaries.insert(boundaryKey);
                        
                        const Plate* platePtr1 = plateMap[plate1];
                        const Plate* platePtr2 = plateMap[plate2];
                        
                        if (platePtr1 && platePtr2) {
                            // Calculate boundary position as midpoint
                            glm::vec3 pos1 = glm::normalize(tile.GetCenter());
                            glm::vec3 pos2 = glm::normalize(tiles[neighborIdx].GetCenter());
                            glm::vec3 boundaryPos = glm::normalize((pos1 + pos2) * 0.5f);
                            
                            // Determine boundary type and stress
                            auto [boundaryType, stress] = DetermineBoundaryType(*platePtr1, *platePtr2, boundaryPos);
                            
                            // Calculate boundary normal
                            glm::vec3 boundaryDirection = glm::normalize(platePtr2->center - platePtr1->center);
                            glm::vec3 boundaryNormal = glm::cross(boundaryPos, boundaryDirection);
                            if (glm::length(boundaryNormal) > 0.001f) {
                                boundaryNormal = glm::normalize(boundaryNormal);
                            } else {
                                boundaryNormal = glm::vec3(0, 1, 0); // fallback
                            }
                            
                            // Create boundary info
                            BoundaryInfo boundary;
                            boundary.plateId1 = plate1;
                            boundary.plateId2 = plate2;
                            boundary.position = boundaryPos;
                            boundary.normal = boundaryNormal;
                            boundary.type = boundaryType;
                            boundary.stress = stress;
                            
                            boundaries.push_back(boundary);
                            
                            std::string typeStr = (boundaryType == BoundaryType::Convergent) ? "convergent" :
                                                 (boundaryType == BoundaryType::Divergent) ? "divergent" : "transform";
                            std::cout << "Boundary " << plate1 << "-" << plate2 
                                      << ": " << typeStr << " (stress: " << stress << ")" << std::endl;
                        }
                    }
                }
            }
        }
    }
    
    std::cout << "Found " << boundaries.size() << " unique plate boundaries" << std::endl;
    return boundaries;
}

std::pair<BoundaryType, float> DetermineBoundaryType(const Plate& plate1, const Plate& plate2, 
                                                    const glm::vec3& boundaryPosition) {
    // Calculate relative movement at boundary
    glm::vec3 relativeVelocity = plate2.movement - plate1.movement;
    
    // Calculate boundary direction and normal
    glm::vec3 boundaryDirection = glm::normalize(plate2.center - plate1.center);
    glm::vec3 boundaryNormal = glm::cross(boundaryPosition, boundaryDirection);
    if (glm::length(boundaryNormal) > 0.001f) {
        boundaryNormal = glm::normalize(boundaryNormal);
    } else {
        boundaryNormal = glm::vec3(0, 1, 0); // fallback
    }
    
    // Project relative velocity onto boundary normal to determine convergence/divergence
    float convergenceSpeed = glm::dot(relativeVelocity, boundaryNormal);
    float relativeSpeed = glm::length(relativeVelocity);
    
    BoundaryType type;
    float stress = 0.0f;
    
    if (std::abs(convergenceSpeed) > relativeSpeed * 0.5f) {
        if (convergenceSpeed > 0) {
            // Plates moving together - convergent boundary
            type = BoundaryType::Convergent;
            stress = std::abs(convergenceSpeed) * 1000.0f; // Base stress factor
            
            // Continental-continental collision creates highest stress
            if (!plate1.isOceanic && !plate2.isOceanic) {
                stress *= 2.0f; // Himalayas-type collision
            } else if (plate1.isOceanic != plate2.isOceanic) {
                stress *= 1.5f; // Andes-type subduction
            }
            // Oceanic-oceanic creates island arcs (base stress)
        } else {
            // Plates moving apart - divergent boundary
            type = BoundaryType::Divergent;
            stress = std::abs(convergenceSpeed) * 500.0f; // Rifting stress
        }
    } else {
        // Plates sliding past each other - transform boundary
        type = BoundaryType::Transform;
        stress = relativeSpeed * 200.0f; // Transform fault stress
    }
    
    return {type, stress};
}

// REMOVED: GetPlateBaseElevation was causing all tiles on a plate to have uniform elevation
// Now we preserve the original terrain variation and only add mountain effects

float CalculateInfluence(float distance, float maxDistance) {
    // Exponential decay function for concentrated mountain ranges
    // Influence drops off quickly with distance from boundary
    if (distance >= maxDistance) {
        return 0.0f;
    }
    
    // Exponential decay: 1.0 at boundary, approaches 0 at maxDistance
    return std::exp(-4.0f * (distance / maxDistance));
}

float CalculateMountainHeight(float stress, float influence, bool isOceanic1, bool isOceanic2) {
    // Base mountain height from stress and influence
    float baseHeight = stress * influence;
    
    // Apply non-linear scaling to create more dramatic peaks
    // Use a quadratic function for more pronounced mountains
    float mountainHeight = baseHeight * baseHeight * 2.0f;
    
    // Adjust based on plate types (geological realism)
    if (!isOceanic1 && !isOceanic2) {
        // Continental-continental collision (highest mountains)
        // Examples: Himalayas, Alps, Appalachians
        mountainHeight *= 3.0f;
    } else if (isOceanic1 != isOceanic2) {
        // Continental-oceanic collision
        // Examples: Andes, Cascades, Japanese Alps
        mountainHeight *= 1.5f;
        
        // Note: The oceanic side would create trenches (handled elsewhere)
    } else {
        // Oceanic-oceanic collision creates island arcs
        // Examples: Aleutians, Japanese islands, Philippines
        mountainHeight *= 1.0f; // Base scaling
    }
    
    return mountainHeight;
}

float ApplyFoldingPattern(const glm::vec3& point, const glm::vec3& boundaryPoint, 
                         const glm::vec3& normal, float distance, float stress) {
    // Create folding pattern perpendicular to collision direction
    // Real mountain ranges show parallel ridges and valleys
    
    // Frequency depends on stress - higher stress creates tighter folds
    float foldFrequency = 8.0f + stress * 3.0f;
    
    // Project point onto folding direction (perpendicular to collision)
    glm::vec3 foldDirection = glm::normalize(glm::cross(normal, boundaryPoint));
    if (glm::length(foldDirection) < 0.001f) {
        foldDirection = glm::vec3(1, 0, 0); // fallback
    }
    
    float projection = glm::dot(point, foldDirection);
    
    // Calculate fold amplitude that decreases with distance from boundary
    float maxAmplitude = 0.1f * stress;
    float amplitude = maxAmplitude * std::exp(-distance * 8.0f);
    
    // Apply sine wave pattern for ridges and valleys
    return amplitude * std::sin(projection * foldFrequency);
}

float ApplyIsostaticAdjustment(float elevation) {
    // Apply crustal thickening effect (isostatic adjustment)
    // Thicker crust "floats" higher on the mantle
    
    const float seaLevel = PlanetParameters().physicalRadiusMeters;
    const float isostaticThreshold = seaLevel + 2000.0f; // 2km above sea level
    if (elevation > isostaticThreshold) {
        // Calculate additional elevation boost from crustal thickening
        float excess = (elevation - isostaticThreshold) / 1000.0f; // Normalize to km
        float isostaticAdjustment = excess * excess * 400.0f; // Non-linear boost in meters
        return elevation + isostaticAdjustment;
    }
    
    return elevation;
}

void GenerateComprehensiveMountains(World* world, const std::vector<Plate>& plates,
                                   std::shared_ptr<ProgressTracker> progressTracker) {
    if (!world || plates.empty()) {
        std::cerr << "Error: Invalid world or no plates for mountain generation" << std::endl;
        return;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.0f, "Starting comprehensive mountain generation...");
    }
    
    const auto& tiles = world->GetTiles();
    std::cout << "Generating comprehensive mountains for " << tiles.size() 
              << " tiles across " << plates.size() << " plates..." << std::endl;
    
    // Step 1: Analyze all plate boundaries
    if (progressTracker) {
        progressTracker->UpdateProgress(0.1f, "Analyzing plate boundaries...");
    }
    
    auto boundaries = AnalyzePlateBoundaries(world, plates);
    
    if (boundaries.empty()) {
        std::cout << "No plate boundaries found - skipping mountain generation" << std::endl;
        return;
    }
    
    // Step 2: Create plate lookup map
    std::map<int, const Plate*> plateMap;
    for (const auto& plate : plates) {
        plateMap[plate.id] = &plate;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.2f, "Calculating elevations for all tiles...");
    }
    
    // Step 3: Calculate comprehensive elevation for ALL tiles
    const float maxInfluenceDistance = 0.25f; // Maximum distance for boundary influence
    
    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
        const auto& tile = tiles[tileIdx];
        int tilePlateId = tile.GetPlateId();
        
        if (tilePlateId < 0) {
            continue; // Skip unassigned tiles
        }
        
        const Plate* tileplate = plateMap[tilePlateId];
        if (!tileplate) continue;
        
        // Start with the tile's existing elevation (preserves terrain variation)
        float newElevation = tile.GetElevation();
        
        glm::vec3 tilePos = glm::normalize(tile.GetCenter());
        
        // Calculate influence from all relevant boundaries
        for (const auto& boundary : boundaries) {
            // Calculate distance from tile to boundary
            float distance = glm::distance(tilePos, boundary.position);
            
            // Only apply influence if within range
            if (distance < maxInfluenceDistance) {
                float influence = CalculateInfluence(distance, maxInfluenceDistance);
                
                if (influence > 0.01f) { // Only process if significant influence
                    const Plate* plate1 = plateMap[boundary.plateId1];
                    const Plate* plate2 = plateMap[boundary.plateId2];
                    
                    if (plate1 && plate2) {
                        float elevationChange = 0.0f;
                        
                        if (boundary.type == BoundaryType::Convergent) {
                            // Mountain formation from collision
                            float normalizedStress = boundary.stress / 1000.0f;
                            float mountainContribution = CalculateMountainHeight(normalizedStress, influence, 
                                                                               plate1->isOceanic, plate2->isOceanic);
                            
                            // Add folding pattern for realistic ridge formation
                            float foldingContribution = ApplyFoldingPattern(tilePos, boundary.position, 
                                                                           boundary.normal, distance, normalizedStress);
                            
                            elevationChange = (mountainContribution + foldingContribution) * 1000.0f; // Convert to meters
                            
                        } else if (boundary.type == BoundaryType::Divergent) {
                            // Rifting creates valleys and lower elevation
                            float normalizedStress = boundary.stress / 500.0f;
                            elevationChange = -normalizedStress * influence * 300.0f; // Rift valleys in meters
                            
                        } else if (boundary.type == BoundaryType::Transform) {
                            // Transform boundaries create moderate relief variation
                            float normalizedStress = boundary.stress / 200.0f;
                            float noise = sin(tilePos.x * 6.0f) * cos(tilePos.z * 6.0f);
                            elevationChange = normalizedStress * influence * noise * 200.0f; // Transform relief in meters
                        }
                        
                        newElevation += elevationChange;
                    }
                }
            }
        }
        
        // Apply isostatic adjustment for crustal thickening effects
        newElevation = ApplyIsostaticAdjustment(newElevation);
        
        // No clamping needed for physical meter values
        
        // Set the new elevation
        const_cast<Tile&>(tile).SetElevation(newElevation);
        
        // Note: Terrain type will be set by the Biome generator based on final elevation
        
        // Report progress periodically
        if (progressTracker && tileIdx % 1000 == 0) {
            float progress = 0.2f + (static_cast<float>(tileIdx) / tiles.size()) * 0.8f;
            progressTracker->UpdateProgress(progress, "Processing tile " + std::to_string(tileIdx) + "/" + std::to_string(tiles.size()));
        }
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Comprehensive mountain generation complete!");
    }
    
    std::cout << "Comprehensive mountain generation complete - processed all " 
              << tiles.size() << " tiles with " << boundaries.size() << " boundaries." << std::endl;
}

} // namespace Generators
} // namespace WorldGen