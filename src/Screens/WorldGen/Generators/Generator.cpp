#include "Generator.h"
#include <cmath>

namespace WorldGen {
namespace Generators {

std::unique_ptr<World> Generator::CreateWorld(const PlanetParameters& params, std::shared_ptr<ProgressTracker> progressTracker) {
    // Create a new world with the specified parameters and progress tracker
    auto world = std::make_unique<World>(params, progressTracker);
    
    // Calculate appropriate subdivision level based on resolution
    int subdivisionLevel = GetSubdivisionLevel(params.resolution);
    
    // Calculate distortion factor (can be a parameter later)
    float distortionFactor = 0.15f; // Default distortion factor
    
    // Generate the world geometry
    world->Generate(subdivisionLevel, distortionFactor);
    
    return world;
}

int Generator::GetSubdivisionLevel(int resolution) {
    // Resolution corresponds roughly to the number of tiles desired
    // Each subdivision level multiplies the number of tiles by ~4
    // Starting with 20 faces in the icosahedron
    
    // Calculate required subdivision level for the desired resolution
    // Number of tiles after n subdivisions ≈ 20 * 4^n
    // So n ≈ log4(resolution / 20)
    if (resolution <= 20) return 0;
    
    double subdivisions = std::log(resolution / 20.0) / std::log(4.0);
    return static_cast<int>(std::ceil(subdivisions));
}

int Generator::CalculateTileCount(int subdivisionLevel) {
    // Each subdivision increases the number of tiles by factor of ~4
    // Starting with 20 faces on the icosahedron
    return static_cast<int>(20 * std::pow(4, subdivisionLevel));
}

} // namespace Generators
} // namespace WorldGen