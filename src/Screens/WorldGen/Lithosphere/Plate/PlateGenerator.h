#pragma once

#include <vector>
#include <memory>
#include <random>
// #include "Lithosphere.h" // REMOVE: Causes circular dependency, forward declare instead
#include "../../Renderers/PlanetData.h" // Updated path from Planet to Renderers
#include "../../Core/WorldGenParameters.h" // Updated path to Core directory
#include "../../Lithosphere/Crust/MountainGenerator.h" // Updated path to Crust directory

namespace WorldGen {

// Forward declare Lithosphere
class Lithosphere;

class PlateGenerator {
public:
    // Use PlanetParameters from WorldGenParameters.h
    PlateGenerator(const PlanetParameters& parameters, uint64_t seed);

    // Getter for the Lithosphere instance
    Lithosphere* GetLithosphere() const { return m_lithosphere.get(); }
    
    // Updated method for calculating elevation with mountain formation - now takes planetVertices
    float CalculateElevationAtPoint(
        const glm::vec3& point,
        const std::vector<std::shared_ptr<TectonicPlate>>& plates,
        const std::vector<glm::vec3>& planetVertices);

private:
    std::unique_ptr<Lithosphere> m_lithosphere; // Added Lithosphere member
    std::unique_ptr<MountainGenerator> m_mountainGenerator; // Add MountainGenerator member
};

} // namespace WorldGen