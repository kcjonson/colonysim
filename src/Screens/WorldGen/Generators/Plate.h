#pragma once

#include <vector>
#include <random>
#include <glm/glm.hpp>
#include <memory>

namespace WorldGen {

// Forward declaration
class ProgressTracker;

namespace Generators {
    class World;

/**
 * @brief Plate size classification
 */
enum class PlateSize {
    Major,      // Large plates (like Pacific, Eurasian)
    Minor       // Smaller plates (like Caribbean, Arabian)
};

/**
 * @brief Simple plate structure for functional approach
 */
struct Plate {
    int id;
    glm::vec3 center;           // Center position on sphere
    glm::vec3 movement;         // Movement vector (tangent to sphere)
    float rotationRate;         // Angular velocity
    bool isOceanic;             // Oceanic vs continental
    PlateSize size;             // Major vs minor plate
    std::vector<int> tileIds;   // Tiles belonging to this plate
};

/**
 * @brief Generate tectonic plates for a world
 * 
 * Creates well-distributed plate centers using Fibonacci sphere distribution
 * with realistic movement patterns based on simulated mantle convection.
 * 
 * @param world The world to generate plates for
 * @param numPlates Number of plates to generate
 * @param seed Random seed
 * @param progressTracker Optional progress tracker for reporting progress
 * @return std::vector<Plate> Generated plates with realistic properties
 */
std::vector<Plate> GeneratePlates(World* world, int numPlates, uint64_t seed, 
                                 std::shared_ptr<ProgressTracker> progressTracker = nullptr);

/**
 * @brief Assign tiles to plates using Voronoi-like regions with noise
 * 
 * Assigns all world tiles to the closest major plate using distance-based
 * assignment with noise for natural, irregular boundaries.
 * 
 * @param world The world containing tiles
 * @param plates The plates to assign tiles to (modified in-place)
 * @param targetTotalPlates Target number of plates (unused but kept for compatibility)
 * @param seed Random seed for boundary noise
 * @param progressTracker Optional progress tracker for reporting progress
 */
void AssignTilesToPlates(World* world, std::vector<Plate>& plates, int targetTotalPlates, uint64_t seed, 
                        std::shared_ptr<ProgressTracker> progressTracker = nullptr);

/**
 * @brief Generate well-distributed points on a sphere using Fibonacci sequence
 * 
 * Creates evenly distributed points using the golden angle method with
 * added randomization to avoid perfect patterns.
 * 
 * @param numSamples Number of points to generate
 * @param seed Random seed for randomization
 * @return std::vector<glm::vec3> Normalized points on unit sphere
 */
std::vector<glm::vec3> GenerateWellDistributedPoints(int numSamples, uint64_t seed);

} // namespace Generators
} // namespace WorldGen