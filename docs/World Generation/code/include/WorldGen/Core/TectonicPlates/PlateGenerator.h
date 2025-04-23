#pragma once

/**
 * @file PlateGenerator.h
 * @brief Generates and simulates tectonic plates for planet formation
 */

#include "WorldGen/Core/TectonicPlates/TectonicPlate.h"
#include "WorldGen/PlanetParameters.h"
#include "WorldGen/ProgressTracker.h"
#include <vector>
#include <memory>
#include <random>

namespace WorldGen {
namespace Core {

/**
 * @brief Generates and simulates tectonic plates for planet formation
 * 
 * This class handles the creation of tectonic plates and simulates their
 * movement and interactions to create realistic terrain features.
 */
class PlateGenerator {
public:
    /**
     * @brief Constructor
     * @param parameters Planet parameters
     * @param seed Random seed for generation
     */
    PlateGenerator(const PlanetParameters& parameters, uint64_t seed);
    
    /**
     * @brief Destructor
     */
    ~PlateGenerator();
    
    /**
     * @brief Generate tectonic plates
     * @param progressTracker Optional progress tracker
     * @return Vector of generated tectonic plates
     */
    std::vector<std::shared_ptr<TectonicPlate>> GeneratePlates(
        std::shared_ptr<ProgressTracker> progressTracker = nullptr);
    
    /**
     * @brief Simulate plate movement and interactions
     * @param plates Vector of tectonic plates
     * @param simulationSteps Number of simulation steps to run
     * @param progressTracker Optional progress tracker
     */
    void SimulatePlateMovement(
        std::vector<std::shared_ptr<TectonicPlate>>& plates,
        int simulationSteps,
        std::shared_ptr<ProgressTracker> progressTracker = nullptr);
    
    /**
     * @brief Analyze plate boundaries to determine interaction types
     * @param plates Vector of tectonic plates
     * @param progressTracker Optional progress tracker
     */
    void AnalyzeBoundaries(
        std::vector<std::shared_ptr<TectonicPlate>>& plates,
        std::shared_ptr<ProgressTracker> progressTracker = nullptr);
    
    /**
     * @brief Generate elevation data based on plate interactions
     * @param plates Vector of tectonic plates
     * @param resolution Resolution of elevation grid
     * @param progressTracker Optional progress tracker
     * @return Elevation grid data
     */
    std::vector<float> GenerateElevationData(
        const std::vector<std::shared_ptr<TectonicPlate>>& plates,
        int resolution,
        std::shared_ptr<ProgressTracker> progressTracker = nullptr);

private:
    PlanetParameters m_parameters;
    std::mt19937_64 m_random;
    
    // Helper methods
    void GeneratePlateCenters(std::vector<glm::vec3>& centers, int numPlates);
    void AssignTilesToPlates(std::vector<std::shared_ptr<TectonicPlate>>& plates, int resolution);
    void GeneratePlateMovements(std::vector<std::shared_ptr<TectonicPlate>>& plates);
    void DetectPlateBoundaries(std::vector<std::shared_ptr<TectonicPlate>>& plates);
    BoundaryType DetermineBoundaryType(const glm::vec3& point, 
                                      const TectonicPlate& plate1, 
                                      const TectonicPlate& plate2);
    float CalculateStressAtBoundary(const PlateBoundary& boundary,
                                   const TectonicPlate& plate1,
                                   const TectonicPlate& plate2);
    float CalculateElevationAtPoint(const glm::vec3& point,
                                   const std::vector<std::shared_ptr<TectonicPlate>>& plates);
};

} // namespace Core
} // namespace WorldGen
