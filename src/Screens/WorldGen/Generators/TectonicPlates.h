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
 * @param world The world to generate plates for
 * @param numPlates Number of plates to generate
 * @param seed Random seed
 * @param progressTracker Optional progress tracker for reporting progress
 * @return std::vector<Plate> Generated plates
 */
std::vector<Plate> GeneratePlates(World* world, int numPlates, uint64_t seed, std::shared_ptr<ProgressTracker> progressTracker = nullptr);

/**
 * @brief Assign tiles to plates and create minor plates at boundaries
 * 
 * @param world The world containing tiles
 * @param plates The plates (major plates in, major+minor out)
 * @param targetTotalPlates Total number of plates desired
 * @param seed Random seed for minor plate generation
 * @param progressTracker Optional progress tracker for reporting progress
 */
void AssignTilesToPlates(World* world, std::vector<Plate>& plates, int targetTotalPlates, uint64_t seed, std::shared_ptr<ProgressTracker> progressTracker = nullptr);

/**
 * @brief Generate mountains at plate boundaries (legacy simple version)
 * 
 * @param world The world containing tiles with plate assignments
 * @param plates The plates with their properties
 * @param progressTracker Optional progress tracker for reporting progress
 * @deprecated Use MountainGenerator::GenerateComprehensiveMountains for enhanced mountain generation
 */
void GenerateMountains(World* world, const std::vector<Plate>& plates, std::shared_ptr<ProgressTracker> progressTracker = nullptr);


} // namespace Generators
} // namespace WorldGen