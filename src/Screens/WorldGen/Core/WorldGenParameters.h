#pragma once

#include <cstdint>
#include <string>

namespace WorldGen {

struct PlanetParameters {
    // Planet properties
    float radius = 1.0f;
    float mass = 1.0f;
    float rotationRate = 1.0f;
    int numTectonicPlates = 8;
    float waterAmount = 0.7f;
    float atmosphereDensity = 1.0f;
    uint64_t planetAge = 4500000000; // ~4.5 billion years
    
    // Star properties
    float starMass = 1.0f;
    float starRadius = 1.0f;
    float starTemperature = 5778.0f; // Sun's temperature in Kelvin
    uint64_t starAge = 5000000000; // ~5 billion years
    
    // Orbital parameters
    float semiMajorAxis = 1.0f; // AU
    float eccentricity = 0.017f;
    
    // Generator properties
    int resolution = 100000; // Terrain resolution (what units are these?)
    uint64_t seed = 12345; // Default seed
};

// Other parameter structures can be added here

} // namespace WorldGen