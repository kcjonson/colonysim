#pragma once

#include <vector>
#include <memory>
#include <random>
#include "TectonicPlate.h"
// #include "../../../Core/ProgressTracker.h"
#include "../Planet/PlanetData.h"

namespace WorldGen {

struct PlanetParameters {
    int numTectonicPlates = 12;
    int resolution = 32;
};

class PlateGenerator {
public:
    PlateGenerator(const PlanetParameters& parameters, uint64_t seed);

    std::vector<std::shared_ptr<TectonicPlate>> GeneratePlates();

    void SimulatePlateMovement(
        std::vector<std::shared_ptr<TectonicPlate>>& plates,
        int simulationSteps);

    // Analyze plate boundaries
    void AnalyzeBoundaries(
        std::vector<std::shared_ptr<TectonicPlate>>& plates);

    // Generate elevation data for the planet
    std::vector<float> GenerateElevationData(
        const std::vector<std::shared_ptr<TectonicPlate>>& plates,
        int resolution);

private:
    void GeneratePlateCenters(std::vector<glm::vec3>& centers, int numPlates);
    void AssignTilesToPlates(std::vector<std::shared_ptr<TectonicPlate>>& plates, int resolution);
    void DetectPlateBoundaries(std::vector<std::shared_ptr<TectonicPlate>>& plates);
    void GeneratePlateMovements(std::vector<std::shared_ptr<TectonicPlate>>& plates);
    
    BoundaryType DetermineBoundaryType(
        const glm::vec3& point,
        const TectonicPlate& plate1,
        const TectonicPlate& plate2);
        
    float CalculateStressAtBoundary(
        const PlateBoundary& boundary,
        const TectonicPlate& plate1,
        const TectonicPlate& plate2);

    float CalculateElevationAtPoint(
        const glm::vec3& point,
        const std::vector<std::shared_ptr<TectonicPlate>>& plates);

private:
    PlanetParameters m_parameters;
    std::mt19937_64 m_random;
};

} // namespace WorldGen 