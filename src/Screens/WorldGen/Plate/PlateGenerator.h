#pragma once

#include <vector>
#include <memory>
#include <random>
// #include "Lithosphere.h" // REMOVE: Causes circular dependency, forward declare instead
#include "../Planet/PlanetData.h"
#include "../WorldGenParameters.h" // Include the new parameters header

namespace WorldGen {

// Forward declare Lithosphere
class Lithosphere;

// Remove the local struct definition
// struct PlanetParameters {
//     int numTectonicPlates = 12;
//     int planetResolution = 32; // Renamed from resolution to avoid conflict if PlanetData has one
//     // Add other parameters as needed
//     // float simulationSpeedFactor = 1.0f;
// };

class PlateGenerator {
public:
    // Use PlanetParameters from WorldGenParameters.h
    PlateGenerator(const PlanetParameters& parameters, uint64_t seed);

    // Getter for the Lithosphere instance
    Lithosphere* GetLithosphere() const { return m_lithosphere.get(); }

private:
    std::unique_ptr<Lithosphere> m_lithosphere; // Added Lithosphere member
};

} // namespace WorldGen