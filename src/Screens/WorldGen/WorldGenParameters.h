#pragma once

namespace WorldGen {

// Parameters for world generation, including plate tectonics
struct PlanetParameters {
    int numTectonicPlates = 12;
    int planetResolution = 32; // Resolution of the base planet mesh
    // Add other parameters as needed
    // float simulationSpeedFactor = 1.0f;
    // float seaLevel = 0.5f; // Example
};

} // namespace WorldGen