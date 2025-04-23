#pragma once

/**
 * @file AtmosphericCirculation.h
 * @brief Simulates global atmospheric circulation patterns
 */

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "WorldGen/PlanetParameters.h"
#include "WorldGen/ProgressTracker.h"

namespace WorldGen {
namespace Core {

/**
 * @brief Represents a circulation cell in the atmosphere
 */
enum class CirculationCellType {
    Hadley,     ///< Tropical cell (0-30 degrees latitude)
    Ferrel,     ///< Mid-latitude cell (30-60 degrees latitude)
    Polar       ///< Polar cell (60-90 degrees latitude)
};

/**
 * @brief Simulates global atmospheric circulation patterns
 * 
 * This class handles the simulation of atmospheric circulation based on
 * the planet's physical properties, generating wind patterns and pressure systems.
 */
class AtmosphericCirculation {
public:
    /**
     * @brief Constructor
     * @param parameters Planet parameters
     */
    explicit AtmosphericCirculation(const PlanetParameters& parameters);
    
    /**
     * @brief Destructor
     */
    ~AtmosphericCirculation();
    
    /**
     * @brief Generate global circulation patterns
     * @param elevationData Terrain elevation data
     * @param resolution Resolution of the grid
     * @param progressTracker Optional progress tracker
     * @return Wind vector grid (3D vectors representing wind direction and strength)
     */
    std::vector<glm::vec3> GenerateCirculation(
        const std::vector<float>& elevationData,
        int resolution,
        std::shared_ptr<ProgressTracker> progressTracker = nullptr);
    
    /**
     * @brief Get air pressure at a specific point
     * @param latitude Latitude in degrees (-90 to 90)
     * @param longitude Longitude in degrees (-180 to 180)
     * @return Air pressure value (normalized 0.0-1.0)
     */
    float GetAirPressureAt(float latitude, float longitude) const;
    
    /**
     * @brief Get wind vector at a specific point
     * @param latitude Latitude in degrees (-90 to 90)
     * @param longitude Longitude in degrees (-180 to 180)
     * @return 3D vector representing wind direction and strength
     */
    glm::vec3 GetWindVectorAt(float latitude, float longitude) const;
    
    /**
     * @brief Get temperature at a specific point
     * @param latitude Latitude in degrees (-90 to 90)
     * @param longitude Longitude in degrees (-180 to 180)
     * @param elevation Elevation at the point
     * @return Temperature in Celsius
     */
    float GetTemperatureAt(float latitude, float longitude, float elevation) const;

private:
    PlanetParameters m_parameters;
    std::vector<glm::vec3> m_windVectors;
    std::vector<float> m_pressureGrid;
    std::vector<float> m_temperatureGrid;
    int m_resolution;
    
    // Helper methods
    void GenerateGlobalWindPatterns(float rotationRate, int resolution);
    void ApplyCoriolisEffect(std::vector<glm::vec3>& windVectors, float rotationRate);
    void ApplyTopographicalEffects(std::vector<glm::vec3>& windVectors, 
                                  const std::vector<float>& elevationData);
    void GeneratePressureSystems(int resolution);
    void GenerateTemperatureMap(int resolution);
    
    // Circulation cell generation
    void GenerateHadleyCell(float planetRadius, float rotationRate);
    void GenerateFerrelCell(float planetRadius, float rotationRate);
    void GeneratePolarCell(float planetRadius, float rotationRate);
    
    // Utility methods
    int GetGridIndex(float latitude, float longitude) const;
    float CalculateBaseTemperature(float latitude) const;
    float CalculatePressureGradient(float latitude) const;
};

} // namespace Core
} // namespace WorldGen
