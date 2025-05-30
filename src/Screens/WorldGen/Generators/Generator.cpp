#include "Generator.h"
#include "Plate.h"
#include "Mountain.h"
#include <cmath>
#include <iostream>

namespace WorldGen {
namespace Generators {

std::unique_ptr<World> Generator::CreateWorld(const PlanetParameters& params, uint64_t seed, std::shared_ptr<ProgressTracker> progressTracker) {
    std::cout << "Starting complete world generation pipeline..." << std::endl;
    
    // Phase 1: Create geometric world (icosahedral subdivision)
    if (progressTracker) {
        progressTracker->UpdateProgress(0.0f, "Creating world geometry...");
    }
    
    auto world = std::make_unique<World>(params, seed, progressTracker);
    
    // Calculate appropriate subdivision level based on resolution
    int subdivisionLevel = GetSubdivisionLevel(params.resolution);
    
    // Calculate distortion factor (can be a parameter later)
    float distortionFactor = 0.05f; 
    
    // Generate the world geometry (this should take us to ~50% progress)
    world->Generate(subdivisionLevel, distortionFactor, progressTracker);
    
    std::cout << "World geometry complete. Generated " << world->GetTileCount() << " tiles." << std::endl;
    
    // Phase 2: Generate tectonic plates
    if (progressTracker) {
        progressTracker->UpdateProgress(0.5f, "Generating tectonic plates...");
    }
    
    std::vector<Plate> plates = GeneratePlates(world.get(), params.numTectonicPlates, seed + 1, progressTracker);
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.7f, "Assigning tiles to plates...");
    }
    
    AssignTilesToPlates(world.get(), plates, params.numTectonicPlates, seed + 2, progressTracker);
    
    std::cout << "Plate generation complete. Created " << plates.size() << " plates." << std::endl;
    
    // Phase 3: Generate mountains based on plate interactions
    if (progressTracker) {
        progressTracker->UpdateProgress(0.8f, "Generating comprehensive mountain systems...");
    }
    
    GenerateComprehensiveMountains(world.get(), plates, progressTracker);
    
    std::cout << "Mountain generation complete." << std::endl;
    
    // Store plate data in the world for visualization
    world->SetPlates(plates);
    
    // TODO: Future phases
    // Phase 4: Climate simulation
    // Phase 5: River generation
    // Phase 6: Biome assignment
    // Phase 7: Final terrain smoothing
    
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "World generation complete!");
    }
    
    std::cout << "Complete world generation pipeline finished successfully." << std::endl;
    
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