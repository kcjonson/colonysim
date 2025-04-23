#pragma once

/**
 * @file SnowAndGlacierCalculator.h
 * @brief Calculates snow accumulation and glacier formation based on climate conditions
 */

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "WorldGen/PlanetParameters.h"
#include "WorldGen/ProgressTracker.h"

namespace WorldGen {
namespace Core {
namespace Climate {

/**
 * @brief Represents snow conditions for a tile
 */
struct SnowData {
    bool hasSeasonalSnow;       ///< Whether the tile has seasonal snow
    float maxSnowDepth;         ///< Maximum snow depth in meters
    int snowMonthsPerYear;      ///< Number of months with snow cover
    bool isPermanentSnow;       ///< Whether snow persists year-round
    bool isGlacier;             ///< Whether the tile contains glacial ice
    float glacierThickness;     ///< Thickness of glacier in meters (0 if no glacier)
    float glacierFlowDirection; ///< Direction of glacier flow in radians
    float glacierFlowSpeed;     ///< Speed of glacier flow in meters/year
    
    SnowData() : 
        hasSeasonalSnow(false),
        maxSnowDepth(0.0f),
        snowMonthsPerYear(0),
        isPermanentSnow(false),
        isGlacier(false),
        glacierThickness(0.0f),
        glacierFlowDirection(0.0f),
        glacierFlowSpeed(0.0f) {}
};

/**
 * @brief Calculates snow and glacier formation based on climate conditions
 * 
 * This class handles the calculation of snow accumulation and glacier formation
 * based on temperature, precipitation, elevation, latitude, and other factors.
 * It determines where snow will form seasonally, where it will persist year-round,
 * and where glaciers will form due to snow compression over time.
 */
class SnowAndGlacierCalculator {
public:
    /**
     * @brief Constructor
     * @param parameters Planet parameters
     */
    explicit SnowAndGlacierCalculator(const PlanetParameters& parameters);
    
    /**
     * @brief Destructor
     */
    ~SnowAndGlacierCalculator();
    
    /**
     * @brief Calculate snow and glacier conditions for the entire planet
     * @param temperatureData Annual mean temperature data for each tile
     * @param temperatureVariationData Seasonal temperature variation data
     * @param precipitationData Annual precipitation data for each tile
     * @param precipitationSeasonalityData Seasonal precipitation variation data
     * @param elevationData Elevation data for each tile
     * @param latitudeData Latitude data for each tile
     * @param resolution Resolution of the grid
     * @param planetAge Age of the planet in billions of years
     * @param progressTracker Optional progress tracker
     * @return Vector of SnowData for each tile
     */
    std::vector<SnowData> CalculateSnowAndGlaciers(
        const std::vector<float>& temperatureData,
        const std::vector<float>& temperatureVariationData,
        const std::vector<float>& precipitationData,
        const std::vector<float>& precipitationSeasonalityData,
        const std::vector<float>& elevationData,
        const std::vector<float>& latitudeData,
        int resolution,
        float planetAge,
        std::shared_ptr<ProgressTracker> progressTracker = nullptr);
    
    /**
     * @brief Calculate snow accumulation for a single tile
     * @param annualMeanTemperature Annual mean temperature in Celsius
     * @param temperatureVariation Seasonal temperature variation
     * @param annualPrecipitation Annual precipitation in mm
     * @param precipitationSeasonality Seasonal precipitation variation
     * @param elevation Elevation in meters
     * @param latitude Latitude in degrees (-90 to 90)
     * @param slopeAngle Slope angle in degrees
     * @param aspect Aspect (direction slope faces) in degrees
     * @return SnowData structure with snow conditions
     */
    SnowData CalculateTileSnowConditions(
        float annualMeanTemperature,
        float temperatureVariation,
        float annualPrecipitation,
        float precipitationSeasonality,
        float elevation,
        float latitude,
        float slopeAngle,
        float aspect);
    
    /**
     * @brief Determine if a location can form glaciers
     * @param snowData Snow data for the tile
     * @param neighborSnowData Snow data for neighboring tiles
     * @param elevation Elevation in meters
     * @param slope Slope angle in degrees
     * @param planetAge Age of the planet in billions of years
     * @return True if glacier can form, false otherwise
     */
    bool CanFormGlacier(
        const SnowData& snowData,
        const std::vector<SnowData>& neighborSnowData,
        float elevation,
        float slope,
        float planetAge);
    
    /**
     * @brief Calculate glacier flow for tiles with glaciers
     * @param snowData Vector of snow data for all tiles
     * @param elevationData Elevation data for each tile
     * @param resolution Resolution of the grid
     * @param progressTracker Optional progress tracker
     */
    void CalculateGlacierFlow(
        std::vector<SnowData>& snowData,
        const std::vector<float>& elevationData,
        int resolution,
        std::shared_ptr<ProgressTracker> progressTracker = nullptr);

private:
    PlanetParameters m_parameters;
    
    // Constants for snow and glacier calculations
    static constexpr float FREEZING_POINT_C = 0.0f;
    static constexpr float MIN_SNOW_TEMP_C = -5.0f;
    static constexpr float MIN_ANNUAL_SNOW_FOR_GLACIER_MM = 200.0f;
    static constexpr float SNOW_TO_WATER_RATIO = 10.0f;
    static constexpr float MIN_YEARS_FOR_GLACIER_FORMATION = 100.0f;
    static constexpr float GLACIER_FLOW_THRESHOLD_SLOPE_DEGREES = 2.0f;
    
    // Helper methods
    float CalculateMonthlyTemperature(
        float annualMeanTemperature,
        float temperatureVariation,
        float latitude,
        int month);
    
    float CalculateMonthlyPrecipitation(
        float annualPrecipitation,
        float precipitationSeasonality,
        float latitude,
        int month);
    
    float CalculateSnowAccumulation(
        float temperature,
        float precipitation);
    
    float CalculateSnowMelt(
        float temperature,
        float snowDepth,
        float latitude,
        float slopeAngle,
        float aspect);
    
    float CalculateGlacierThickness(
        float snowAccumulationRate,
        float planetAge,
        float elevation,
        float latitude);
    
    glm::vec2 CalculateGlacierFlowVector(
        float glacierThickness,
        float slopeAngle,
        float aspect);
    
    std::vector<int> GetNeighborIndices(int index, int resolution);
};

} // namespace Climate
} // namespace Core
} // namespace WorldGen
