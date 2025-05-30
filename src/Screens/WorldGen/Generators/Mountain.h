#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Plate.h"

namespace WorldGen {

// Forward declaration
class ProgressTracker;

namespace Generators {

// Forward declaration
class World;

/**
 * @brief Boundary type classification for mountain formation
 */
enum class BoundaryType {
    Convergent,  // Collision - creates mountains
    Divergent,   // Spreading - creates rifts/valleys
    Transform    // Sliding - creates moderate relief
};

/**
 * @brief Boundary information for mountain generation
 */
struct BoundaryInfo {
    int plateId1;
    int plateId2;
    BoundaryType type;
    float stress;           // Tectonic stress magnitude
    glm::vec3 position;     // Boundary position on sphere
    glm::vec3 normal;       // Boundary normal direction
};

/**
 * @brief Generate comprehensive mountain systems based on plate tectonics
 * 
 * This function implements advanced mountain formation using geological principles:
 * - Distance-based influence from plate boundaries
 * - Non-linear height calculations for realistic peaks
 * - Folding patterns for ridges and valleys
 * - Isostatic adjustment for crustal thickening
 * - Different formation types based on plate interactions
 * 
 * @param world The world containing tiles to modify
 * @param plates The tectonic plates with movement data
 * @param progressTracker Optional progress tracking
 */
void GenerateComprehensiveMountains(World* world, 
                                   const std::vector<Plate>& plates,
                                   std::shared_ptr<ProgressTracker> progressTracker = nullptr);

/**
 * @brief Analyze plate boundaries to determine interaction types and stress
 * 
 * @param world The world containing tiles
 * @param plates The tectonic plates
 * @return Vector of boundary information with positions and stress calculations
 */
std::vector<BoundaryInfo> AnalyzePlateBoundaries(World* world, 
                                                const std::vector<Plate>& plates);

/**
 * @brief Calculate mountain height using geological principles
 * 
 * Implements non-linear scaling and plate-type-specific factors:
 * - Continental-continental collisions create highest mountains
 * - Oceanic-continental creates medium mountains and trenches
 * - Oceanic-oceanic creates island arcs
 * 
 * @param stress Tectonic stress magnitude
 * @param influence Distance-based influence factor (0.0-1.0)
 * @param isOceanic1 Whether first plate is oceanic
 * @param isOceanic2 Whether second plate is oceanic
 * @return Mountain height contribution
 */
float CalculateMountainHeight(float stress, float influence, bool isOceanic1, bool isOceanic2);

/**
 * @brief Apply folding pattern effects for parallel ridges and valleys
 * 
 * Creates realistic mountain structure by simulating rock layer folding
 * under compression, resulting in parallel ridges perpendicular to
 * the collision direction.
 * 
 * @param point The point to calculate folding for
 * @param boundaryPoint Nearest boundary point
 * @param normal Boundary normal direction
 * @param distance Distance from boundary
 * @param stress Tectonic stress magnitude
 * @return Folding pattern elevation contribution
 */
float ApplyFoldingPattern(const glm::vec3& point, 
                         const glm::vec3& boundaryPoint, 
                         const glm::vec3& normal, 
                         float distance, 
                         float stress);

/**
 * @brief Calculate distance-based influence using exponential decay
 * 
 * Mountain formation influence decreases exponentially with distance
 * from plate boundaries, creating concentrated mountain ranges.
 * 
 * @param distance Distance from boundary
 * @param maxDistance Maximum influence distance
 * @return Influence factor (0.0-1.0)
 */
float CalculateInfluence(float distance, float maxDistance);

/**
 * @brief Apply isostatic adjustment for crustal thickening effects
 * 
 * Simulates how thickened crust "floats" higher on the mantle,
 * creating elevated plateaus in mountainous regions.
 * 
 * @param elevation Current elevation
 * @return Adjusted elevation with isostatic effects
 */
float ApplyIsostaticAdjustment(float elevation);

/**
 * @brief Get base elevation for a plate based on its type
 * 
 * Oceanic plates are generally at lower elevations (below sea level),
 * while continental plates are at higher elevations (above sea level).
 * 
 * @param plate The tectonic plate
 * @return Base elevation (0.0-1.0)
 */
float GetPlateBaseElevation(const Plate& plate);

/**
 * @brief Determine boundary type and stress from plate movement
 * 
 * Analyzes relative plate movement to classify boundary interactions
 * and calculate the resulting tectonic stress.
 * 
 * @param plate1 First plate
 * @param plate2 Second plate
 * @param boundaryPosition Position of the boundary
 * @return Boundary type and stress magnitude
 */
std::pair<BoundaryType, float> DetermineBoundaryType(const Plate& plate1, 
                                                    const Plate& plate2, 
                                                    const glm::vec3& boundaryPosition);

} // namespace Generators
} // namespace WorldGen