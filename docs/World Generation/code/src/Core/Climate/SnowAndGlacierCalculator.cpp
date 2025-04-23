#include "WorldGen/Core/Climate/SnowAndGlacierCalculator.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace WorldGen {
namespace Core {
namespace Climate {

// Constants for calculations
constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

SnowAndGlacierCalculator::SnowAndGlacierCalculator(const PlanetParameters& parameters)
    : m_parameters(parameters)
{
}

SnowAndGlacierCalculator::~SnowAndGlacierCalculator()
{
}

std::vector<SnowData> SnowAndGlacierCalculator::CalculateSnowAndGlaciers(
    const std::vector<float>& temperatureData,
    const std::vector<float>& temperatureVariationData,
    const std::vector<float>& precipitationData,
    const std::vector<float>& precipitationSeasonalityData,
    const std::vector<float>& elevationData,
    const std::vector<float>& latitudeData,
    int resolution,
    float planetAge,
    std::shared_ptr<ProgressTracker> progressTracker)
{
    // Initialize progress tracking if provided
    if (progressTracker) {
        progressTracker->StartPhase("Calculating Snow and Glaciers", 0.1f);
    }
    
    // Create result vector
    std::vector<SnowData> snowData(temperatureData.size());
    
    // First pass: Calculate basic snow conditions for each tile
    for (size_t i = 0; i < temperatureData.size(); ++i) {
        // Assume flat terrain for initial calculation, will refine later
        snowData[i] = CalculateTileSnowConditions(
            temperatureData[i],
            temperatureVariationData[i],
            precipitationData[i],
            precipitationSeasonalityData[i],
            elevationData[i],
            latitudeData[i],
            0.0f,  // Placeholder for slope
            0.0f   // Placeholder for aspect
        );
        
        // Update progress
        if (progressTracker && i % 1000 == 0) {
            float progress = static_cast<float>(i) / temperatureData.size() * 0.5f;
            progressTracker->UpdateProgress(progress, "Calculating snow conditions");
        }
    }
    
    // Second pass: Determine glacier formation based on snow conditions and neighbors
    for (size_t i = 0; i < temperatureData.size(); ++i) {
        if (snowData[i].isPermanentSnow) {
            // Get neighbors for context
            std::vector<int> neighborIndices = GetNeighborIndices(static_cast<int>(i), resolution);
            std::vector<SnowData> neighborSnowData;
            
            for (int neighborIndex : neighborIndices) {
                if (neighborIndex >= 0 && neighborIndex < static_cast<int>(snowData.size())) {
                    neighborSnowData.push_back(snowData[neighborIndex]);
                }
            }
            
            // Calculate slope from elevation data and neighbors
            float slope = 0.0f;  // Placeholder, would calculate from elevation differences
            
            // Check if this location can form a glacier
            if (CanFormGlacier(snowData[i], neighborSnowData, elevationData[i], slope, planetAge)) {
                snowData[i].isGlacier = true;
                
                // Calculate glacier thickness based on accumulation rate and planet age
                snowData[i].glacierThickness = CalculateGlacierThickness(
                    snowData[i].maxSnowDepth / SNOW_TO_WATER_RATIO,  // Convert snow depth to water equivalent
                    planetAge,
                    elevationData[i],
                    latitudeData[i]
                );
            }
        }
        
        // Update progress
        if (progressTracker && i % 1000 == 0) {
            float progress = 0.5f + static_cast<float>(i) / temperatureData.size() * 0.3f;
            progressTracker->UpdateProgress(progress, "Determining glacier formation");
        }
    }
    
    // Third pass: Calculate glacier flow for tiles with glaciers
    CalculateGlacierFlow(snowData, elevationData, resolution, progressTracker);
    
    // Complete progress phase
    if (progressTracker) {
        progressTracker->CompletePhase();
    }
    
    return snowData;
}

SnowData SnowAndGlacierCalculator::CalculateTileSnowConditions(
    float annualMeanTemperature,
    float temperatureVariation,
    float annualPrecipitation,
    float precipitationSeasonality,
    float elevation,
    float latitude,
    float slopeAngle,
    float aspect)
{
    SnowData result;
    
    // Calculate monthly conditions to determine snow accumulation and persistence
    float totalSnowAccumulation = 0.0f;
    float maxMonthlySnowDepth = 0.0f;
    int snowMonths = 0;
    float currentSnowDepth = 0.0f;
    
    for (int month = 0; month < 12; ++month) {
        // Calculate temperature and precipitation for this month
        float monthlyTemperature = CalculateMonthlyTemperature(
            annualMeanTemperature,
            temperatureVariation,
            latitude,
            month
        );
        
        float monthlyPrecipitation = CalculateMonthlyPrecipitation(
            annualPrecipitation,
            precipitationSeasonality,
            latitude,
            month
        );
        
        // Calculate snow accumulation for this month
        float monthlySnowAccumulation = CalculateSnowAccumulation(
            monthlyTemperature,
            monthlyPrecipitation
        );
        
        // Calculate snow melt for this month
        float monthlySnowMelt = CalculateSnowMelt(
            monthlyTemperature,
            currentSnowDepth,
            latitude,
            slopeAngle,
            aspect
        );
        
        // Update current snow depth
        currentSnowDepth += monthlySnowAccumulation - monthlySnowMelt;
        currentSnowDepth = std::max(0.0f, currentSnowDepth);
        
        // Track maximum snow depth
        maxMonthlySnowDepth = std::max(maxMonthlySnowDepth, currentSnowDepth);
        
        // Count months with snow
        if (currentSnowDepth > 0.0f) {
            snowMonths++;
        }
        
        // Track total accumulation
        totalSnowAccumulation += monthlySnowAccumulation;
    }
    
    // Set basic snow properties
    result.maxSnowDepth = maxMonthlySnowDepth;
    result.snowMonthsPerYear = snowMonths;
    result.hasSeasonalSnow = snowMonths > 0;
    
    // Determine if snow is permanent (persists year-round)
    // This is a simplification - in reality would need to model multiple years
    result.isPermanentSnow = (snowMonths == 12) && (maxMonthlySnowDepth > 0.1f);  // At least 10cm
    
    return result;
}

bool SnowAndGlacierCalculator::CanFormGlacier(
    const SnowData& snowData,
    const std::vector<SnowData>& neighborSnowData,
    float elevation,
    float slope,
    float planetAge)
{
    // Basic requirements for glacier formation
    if (!snowData.isPermanentSnow) {
        return false;
    }
    
    // Check if annual snow accumulation is sufficient
    float annualSnowWaterEquivalent = snowData.maxSnowDepth / SNOW_TO_WATER_RATIO;
    if (annualSnowWaterEquivalent < MIN_ANNUAL_SNOW_FOR_GLACIER_MM / 1000.0f) {  // Convert mm to meters
        return false;
    }
    
    // Check if the planet is old enough for glaciers to have formed
    if (planetAge * 1e9f < MIN_YEARS_FOR_GLACIER_FORMATION) {  // Convert billions of years to years
        return false;
    }
    
    // Check if slope is suitable for glacier formation
    // Too steep and snow will avalanche, too flat and ice won't flow
    if (slope > 45.0f) {
        return false;
    }
    
    // Check if there's a critical mass of permanent snow in the area
    int permanentSnowNeighbors = 0;
    for (const auto& neighbor : neighborSnowData) {
        if (neighbor.isPermanentSnow) {
            permanentSnowNeighbors++;
        }
    }
    
    // Need at least some neighbors with permanent snow to form a glacier
    // This ensures glaciers form in areas with substantial snow fields, not isolated patches
    if (permanentSnowNeighbors < 2) {
        return false;
    }
    
    return true;
}

void SnowAndGlacierCalculator::CalculateGlacierFlow(
    std::vector<SnowData>& snowData,
    const std::vector<float>& elevationData,
    int resolution,
    std::shared_ptr<ProgressTracker> progressTracker)
{
    // Count glacier tiles for progress tracking
    int glacierTileCount = 0;
    for (const auto& tile : snowData) {
        if (tile.isGlacier) {
            glacierTileCount++;
        }
    }
    
    if (glacierTileCount == 0) {
        return;  // No glaciers to process
    }
    
    int processedTiles = 0;
    
    // Calculate flow for each glacier tile
    for (size_t i = 0; i < snowData.size(); ++i) {
        if (snowData[i].isGlacier) {
            // Get neighbors for elevation gradient
            std::vector<int> neighborIndices = GetNeighborIndices(static_cast<int>(i), resolution);
            
            // Find steepest downhill gradient
            float lowestElevation = elevationData[i];
            int lowestNeighborIndex = -1;
            
            for (int neighborIndex : neighborIndices) {
                if (neighborIndex >= 0 && neighborIndex < static_cast<int>(elevationData.size())) {
                    if (elevationData[neighborIndex] < lowestElevation) {
                        lowestElevation = elevationData[neighborIndex];
                        lowestNeighborIndex = neighborIndex;
                    }
                }
            }
            
            // Calculate slope and aspect
            float slope = 0.0f;
            float aspect = 0.0f;
            
            if (lowestNeighborIndex >= 0) {
                // Calculate slope in degrees
                float elevationDifference = elevationData[i] - lowestElevation;
                float horizontalDistance = 1000.0f;  // Assume 1km between grid points (adjust as needed)
                slope = std::atan(elevationDifference / horizontalDistance) * RAD_TO_DEG;
                
                // Calculate aspect (direction of flow) in radians
                // This is a simplified calculation - would need actual coordinates in a real implementation
                int dx = (lowestNeighborIndex % resolution) - (static_cast<int>(i) % resolution);
                int dy = (lowestNeighborIndex / resolution) - (static_cast<int>(i) / resolution);
                aspect = std::atan2(dy, dx);
            }
            
            // Calculate flow vector
            glm::vec2 flowVector = CalculateGlacierFlowVector(
                snowData[i].glacierThickness,
                slope,
                aspect
            );
            
            // Set flow properties
            snowData[i].glacierFlowDirection = aspect;
            snowData[i].glacierFlowSpeed = glm::length(flowVector);
            
            // Update progress
            processedTiles++;
            if (progressTracker && processedTiles % 100 == 0) {
                float progress = 0.8f + static_cast<float>(processedTiles) / glacierTileCount * 0.2f;
                progressTracker->UpdateProgress(progress, "Calculating glacier flow");
            }
        }
    }
}

float SnowAndGlacierCalculator::CalculateMonthlyTemperature(
    float annualMeanTemperature,
    float temperatureVariation,
    float latitude,
    int month)
{
    // Simple sinusoidal model for temperature variation throughout the year
    // Northern hemisphere: coldest in January (month 0), warmest in July (month 6)
    // Southern hemisphere: opposite pattern
    
    float monthFraction = month / 12.0f;
    float phaseShift = (latitude >= 0) ? 0.0f : 0.5f;  // Shift by half a year for southern hemisphere
    
    // Temperature variation throughout the year
    float temperatureOffset = temperatureVariation * std::sin(2.0f * PI * (monthFraction + phaseShift));
    
    return annualMeanTemperature + temperatureOffset;
}

float SnowAndGlacierCalculator::CalculateMonthlyPrecipitation(
    float annualPrecipitation,
    float precipitationSeasonality,
    float latitude,
    int month)
{
    // Monthly fraction of annual precipitation
    float baseFraction = 1.0f / 12.0f;
    
    // Apply seasonality
    float monthFraction = month / 12.0f;
    float phaseShift = (latitude >= 0) ? 0.0f : 0.5f;  // Shift by half a year for southern hemisphere
    
    // Precipitation variation throughout the year
    float variationFactor = 1.0f + precipitationSeasonality * std::sin(2.0f * PI * (monthFraction + phaseShift));
    
    // Ensure the total annual precipitation is preserved
    float normalizedFactor = variationFactor / (1.0f + precipitationSeasonality * 2.0f / PI);
    
    return annualPrecipitation * baseFraction * normalizedFactor;
}

float SnowAndGlacierCalculator::CalculateSnowAccumulation(
    float temperature,
    float precipitation)
{
    // No snow accumulation above freezing
    if (temperature > FREEZING_POINT_C) {
        return 0.0f;
    }
    
    // Calculate fraction of precipitation that falls as snow
    float snowFraction = 1.0f;
    
    // In the temperature range between MIN_SNOW_TEMP_C and FREEZING_POINT_C,
    // there's a mix of snow and rain
    if (temperature > MIN_SNOW_TEMP_C) {
        snowFraction = (FREEZING_POINT_C - temperature) / (FREEZING_POINT_C - MIN_SNOW_TEMP_C);
    }
    
    // Convert precipitation to snow depth (snow is less dense than water)
    // Typical snow-to-water ratio ranges from 10:1 to 20:1 depending on temperature
    float snowToWaterRatio = SNOW_TO_WATER_RATIO;
    
    // Colder temperatures produce lighter, fluffier snow with higher snow-to-water ratio
    if (temperature < -10.0f) {
        snowToWaterRatio = 15.0f;  // Lighter, fluffier snow in very cold conditions
    }
    
    return precipitation * snowFraction * snowToWaterRatio;
}

float SnowAndGlacierCalculator::CalculateSnowMelt(
    float temperature,
    float snowDepth,
    float latitude,
    float slopeAngle,
    float aspect)
{
    // No snow to melt
    if (snowDepth <= 0.0f) {
        return 0.0f;
    }
    
    // No melt below freezing
    if (temperature <= FREEZING_POINT_C) {
        return 0.0f;
    }
    
    // Basic melt rate based on temperature
    // Typical values: 3-5mm water equivalent per degree C per day
    // For a month, multiply by ~30 days
    float baseMeltRate = 4.0f * 30.0f / 1000.0f;  // Convert mm to meters
    float meltAmount = baseMeltRate * (temperature - FREEZING_POINT_C);
    
    // Adjust for solar radiation based on slope, aspect, and latitude
    // South-facing slopes in northern hemisphere (or north-facing in southern) get more sun
    float slopeRadians = slopeAngle * DEG_TO_RAD;
    float latitudeRadians = latitude * DEG_TO_RAD;
    
    // Calculate solar factor (simplified)
    float solarFactor = 1.0f;
    
    if (std::abs(slopeAngle) > 0.1f) {  // Only calculate for non-flat terrain
        // In northern hemisphere, south-facing slopes (aspect ~180°) get more sun
        // In southern hemisphere, north-facing slopes (aspect ~0°) get more sun
        float optimalAspect = (latitude >= 0) ? PI : 0.0f;
        float aspectDifference = std::abs(aspect - optimalAspect);
        if (aspectDifference > PI) {
            aspectDifference = 2.0f * PI - aspectDifference;
        }
        
        // Adjust solar factor based on how close the aspect is to optimal
        float aspectFactor = 1.0f - aspectDifference / PI * 0.5f;  // 0.5-1.0 range
        
        // Steeper slopes aligned with optimal aspect get more direct sunlight
        solarFactor = 1.0f + (aspectFactor - 0.5f) * std::sin(slopeRadians);
    }
    
    // Apply solar factor to melt amount
    meltAmount *= solarFactor;
    
    // Limit melt to available snow
    return std::min(meltAmount, snowDepth);
}

float SnowAndGlacierCalculator::CalculateGlacierThickness(
    float snowAccumulationRate,
    float planetAge,
    float elevation,
    float latitude)
{
    // Basic glacier thickness model based on accumulation rate and time
    // This is a simplified model - real glacier thickness depends on many factors
    
    // Convert planet age to years
    float yearsOfAccumulation = planetAge * 1e9f;
    
    // Cap the effective accumulation time (glaciers reach equilibrium)
    float effectiveYears = std::min(yearsOfAccumulation, 10000.0f);
    
    // Base thickness calculation
    float baseThickness = snowAccumulationRate * effectiveYears * 0.1f;  // Scaling factor
    
    // Apply elevation factor (thicker at higher elevations)
    float elevationFactor = 1.0f + (elevation / 5000.0f);  // Normalize to typical mountain heights
    
    // Apply latitude factor (thicker at higher latitudes)
    float latitudeFactor = 1.0f + std::pow(std::abs(latitude) / 90.0f, 2.0f);
    
    // Calculate final thickness
    float thickness = baseThickness * elevationFactor * latitudeFactor;
    
    // Apply reasonable limits
    return std::min(std::max(thickness, 10.0f), 1000.0f);  // 10m to 1000m range
}

glm::vec2 SnowAndGlacierCalculator::CalculateGlacierFlowVector(
    float glacierThickness,
    float slopeAngle,
    float aspect)
{
    // No flow if slope is too small
    if (slopeAngle < GLACIER_FLOW_THRESHOLD_SLOPE_DEGREES) {
        return glm::vec2(0.0f, 0.0f);
    }
    
    // Convert slope to radians
    float slopeRadians = slopeAngle * DEG_TO_RAD;
    
    // Basic glacier flow speed model
    // Flow speed increases with thickness and slope
    // Typical glacier flow speeds range from a few meters to several hundred meters per year
    
    // Glen's Flow Law (simplified): velocity ~ (thickness^4) * (sin(slope))
    float flowSpeed = std::pow(glacierThickness / 100.0f, 4.0f) * std::sin(slopeRadians) * 50.0f;
    
    // Apply reasonable limits
    flowSpeed = std::min(std::max(flowSpeed, 1.0f), 500.0f);  // 1-500 meters per year
    
    // Calculate flow vector components
    float vx = flowSpeed * std::cos(aspect);
    float vy = flowSpeed * std::sin(aspect);
    
    return glm::vec2(vx, vy);
}

std::vector<int> SnowAndGlacierCalculator::GetNeighborIndices(int index, int resolution)
{
    std::vector<int> neighbors;
    
    // Calculate 2D coordinates
    int x = index % resolution;
    int y = index / resolution;
    
    // Check all 8 neighbors
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            // Skip the center point
            if (dx == 0 && dy == 0) {
                continue;
            }
            
            int nx = x + dx;
            int ny = y + dy;
            
            // Check bounds
            if (nx >= 0 && nx < resolution && ny >= 0 && ny < resolution) {
                int neighborIndex = ny * resolution + nx;
                neighbors.push_back(neighborIndex);
            }
        }
    }
    
    return neighbors;
}

} // namespace Climate
} // namespace Core
} // namespace WorldGen
