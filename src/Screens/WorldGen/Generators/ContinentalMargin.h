#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <memory>

namespace WorldGen {

// Forward declaration
class ProgressTracker;

namespace Generators {
    class World;
    struct Plate;

/**
 * @brief Continental margin types based on tectonic setting
 */
enum class MarginType {
    Passive,    // Divergent/rifted margins (Atlantic-type)
    Active      // Convergent margins with subduction (Pacific-type)
};

/**
 * @brief Parameters for realistic continental margin formation
 */
struct ContinentalMarginParams {
    // Isostatic parameters
    float mantleDensity = 3.3f;           // g/cm³
    float continentalCrustDensity = 2.7f; // g/cm³  
    float oceanicCrustDensity = 3.0f;     // g/cm³
    float sedimentDensity = 2.5f;         // g/cm³
    
    // Crustal thickness ranges
    float continentalCrustThickness = 35.0f; // km (average)
    float oceanicCrustThickness = 7.0f;      // km (average)
    
    // Continental shelf parameters
    float shelfBreakDepth = 0.14f;        // Normalized (140m real depth)
    float maxShelfWidth = 0.15f;          // Normalized distance from coast
    float thermalSubsidenceRate = 0.02f;  // Rate of cooling subsidence
    
    // Sediment parameters  
    float sedimentationRate = 0.01f;      // Thickness per time unit
    float sedimentLoadingFactor = 0.6f;   // Subsidence factor from loading
    
    // Subduction zone parameters
    float trenchDepth = 0.25f;            // Normalized depth for trenches
    float subductionAngle = 45.0f;        // Degrees
    float forearcBasinWidth = 0.08f;      // Distance from trench to arc
};

/**
 * @brief Create realistic continental margins with geological processes
 * 
 * Implements real-world processes:
 * - Isostatic equilibrium based on Airy model
 * - Continental shelf formation for passive margins
 * - Subduction trenches and accretionary wedges for active margins  
 * - Sediment deposition with compaction subsidence
 * - Thermal subsidence for cooling lithosphere
 * 
 * @param world The world containing tiles to modify
 * @param plates The tectonic plates with oceanic/continental types
 * @param params Parameters controlling margin formation processes
 * @param seed Random seed for natural variation
 * @param progressTracker Optional progress tracking
 */
void CreateRealisticContinentalMargins(World* world, const std::vector<Plate>& plates, 
                                     const ContinentalMarginParams& params, uint64_t seed,
                                     std::shared_ptr<ProgressTracker> progressTracker = nullptr);

/**
 * @brief Calculate isostatic elevation based on crustal properties
 * 
 * Uses Airy isostatic model: thicker, less dense crust floats higher
 * 
 * @param crustThickness Thickness of crust in km
 * @param crustDensity Density of crust in g/cm³
 * @param params Continental margin parameters
 * @return float Normalized elevation (0-1 scale)
 */
float CalculateIsostaticElevation(float crustThickness, float crustDensity, 
                                const ContinentalMarginParams& params);

/**
 * @brief Determine continental margin type based on plate boundary proximity
 * 
 * @param world The world containing tiles
 * @param plates The tectonic plates
 * @param tileIndex Index of tile to classify
 * @return MarginType The type of continental margin
 */
MarginType DetermineMarginType(World* world, const std::vector<Plate>& plates, int tileIndex);

/**
 * @brief Apply continental shelf formation for passive margins
 * 
 * Creates realistic shelf profile with:
 * - Gentle slope from shore to shelf break
 * - Thermal subsidence from lithospheric cooling
 * - Sediment progradation and loading subsidence
 * 
 * @param world The world containing tiles
 * @param plates The tectonic plates
 * @param params Formation parameters
 * @param seed Random seed
 */
void FormPassiveMarginShelves(World* world, const std::vector<Plate>& plates,
                            const ContinentalMarginParams& params, uint64_t seed);

/**
 * @brief Apply subduction zone features for active margins
 * 
 * Creates:
 * - Deep ocean trenches at convergent boundaries
 * - Accretionary wedge uplift
 * - Forearc basin subsidence
 * 
 * @param world The world containing tiles  
 * @param plates The tectonic plates
 * @param params Formation parameters
 * @param seed Random seed
 */
void FormActiveMarginTrenches(World* world, const std::vector<Plate>& plates,
                            const ContinentalMarginParams& params, uint64_t seed);

} // namespace Generators
} // namespace WorldGen