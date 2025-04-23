#include "WorldGen/Core/Climate/AtmosphericCirculation.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace WorldGen {
namespace Core {

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

AtmosphericCirculation::AtmosphericCirculation(const PlanetParameters& parameters)
    : m_parameters(parameters)
    , m_resolution(0)
{
}

AtmosphericCirculation::~AtmosphericCirculation()
{
}

std::vector<glm::vec3> AtmosphericCirculation::GenerateCirculation(
    const std::vector<float>& elevationData,
    int resolution,
    std::shared_ptr<ProgressTracker> progressTracker)
{
    // Initialize progress tracking if provided
    if (progressTracker) {
        progressTracker->StartPhase("Generating Atmospheric Circulation", 0.15f);
    }
    
    m_resolution = resolution;
    
    // Initialize grids
    int gridSize = resolution * resolution * 6; // 6 faces for cube mapping
    m_windVectors.resize(gridSize, glm::vec3(0.0f));
    m_pressureGrid.resize(gridSize, 0.0f);
    m_temperatureGrid.resize(gridSize, 0.0f);
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.1f, "Initializing atmospheric model");
    }
    
    // Generate global temperature map
    GenerateTemperatureMap(resolution);
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.3f, "Generating temperature patterns");
    }
    
    // Generate pressure systems
    GeneratePressureSystems(resolution);
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.5f, "Generating pressure systems");
    }
    
    // Generate global wind patterns based on planet rotation
    GenerateGlobalWindPatterns(m_parameters.rotationRate, resolution);
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.7f, "Generating wind patterns");
    }
    
    // Apply Coriolis effect based on planet rotation
    ApplyCoriolisEffect(m_windVectors, m_parameters.rotationRate);
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.9f, "Applying Coriolis effect");
    }
    
    // Apply topographical effects
    ApplyTopographicalEffects(m_windVectors, elevationData);
    
    // Complete progress phase
    if (progressTracker) {
        progressTracker->CompletePhase();
    }
    
    return m_windVectors;
}

float AtmosphericCirculation::GetAirPressureAt(float latitude, float longitude) const
{
    if (m_pressureGrid.empty()) {
        return 0.5f; // Default if not generated yet
    }
    
    int index = GetGridIndex(latitude, longitude);
    if (index >= 0 && index < m_pressureGrid.size()) {
        return m_pressureGrid[index];
    }
    
    return 0.5f; // Default fallback
}

glm::vec3 AtmosphericCirculation::GetWindVectorAt(float latitude, float longitude) const
{
    if (m_windVectors.empty()) {
        return glm::vec3(0.0f); // Default if not generated yet
    }
    
    int index = GetGridIndex(latitude, longitude);
    if (index >= 0 && index < m_windVectors.size()) {
        return m_windVectors[index];
    }
    
    return glm::vec3(0.0f); // Default fallback
}

float AtmosphericCirculation::GetTemperatureAt(float latitude, float longitude, float elevation) const
{
    if (m_temperatureGrid.empty()) {
        // Calculate based on latitude if grid not available
        return CalculateBaseTemperature(latitude) - (elevation * 6.5f); // Temperature drops ~6.5°C per km
    }
    
    int index = GetGridIndex(latitude, longitude);
    if (index >= 0 && index < m_temperatureGrid.size()) {
        // Apply elevation adjustment to stored temperature
        return m_temperatureGrid[index] - (elevation * 6.5f);
    }
    
    // Fallback to calculation
    return CalculateBaseTemperature(latitude) - (elevation * 6.5f);
}

void AtmosphericCirculation::GenerateGlobalWindPatterns(float rotationRate, int resolution)
{
    // This method generates the basic global wind patterns based on the
    // planet's rotation rate and the pressure gradient from equator to poles
    
    // Generate the three main circulation cells
    GenerateHadleyCell(m_parameters.planetRadius, rotationRate);
    GenerateFerrelCell(m_parameters.planetRadius, rotationRate);
    GeneratePolarCell(m_parameters.planetRadius, rotationRate);
    
    // Iterate through the grid
    for (int i = 0; i < m_windVectors.size(); ++i) {
        // Convert grid index to latitude/longitude
        // This is a simplified conversion - a real implementation would use proper mapping
        int x = i % m_resolution;
        int y = (i / m_resolution) % m_resolution;
        int face = i / (m_resolution * m_resolution);
        
        // Convert to normalized coordinates (-1 to 1)
        float nx = (x / static_cast<float>(m_resolution)) * 2.0f - 1.0f;
        float ny = (y / static_cast<float>(m_resolution)) * 2.0f - 1.0f;
        
        // Convert to spherical coordinates (simplified)
        float longitude = nx * 180.0f;
        float latitude = ny * 90.0f;
        
        // Adjust based on cube face (simplified)
        if (face == 1) longitude += 90.0f;
        else if (face == 2) longitude += 180.0f;
        else if (face == 3) longitude += 270.0f;
        else if (face == 4) latitude = 90.0f;
        else if (face == 5) latitude = -90.0f;
        
        // Normalize to -90 to 90 for latitude, -180 to 180 for longitude
        latitude = std::max(-90.0f, std::min(90.0f, latitude));
        longitude = fmod(longitude + 540.0f, 360.0f) - 180.0f;
        
        // Determine which cell this point is in
        CirculationCellType cellType;
        float absLat = std::abs(latitude);
        
        if (absLat < 30.0f) {
            cellType = CirculationCellType::Hadley;
        } else if (absLat < 60.0f) {
            cellType = CirculationCellType::Ferrel;
        } else {
            cellType = CirculationCellType::Polar;
        }
        
        // Base wind direction depends on cell type and hemisphere
        glm::vec3 windDir(0.0f);
        
        switch (cellType) {
            case CirculationCellType::Hadley:
                // Easterly trade winds near surface in tropics
                // Direction is opposite in each hemisphere
                windDir.x = (latitude > 0) ? -1.0f : 1.0f; // East-west component
                windDir.y = 0.0f; // North-south component is minimal at surface
                break;
                
            case CirculationCellType::Ferrel:
                // Westerly winds in mid-latitudes
                // Direction is the same in both hemispheres
                windDir.x = 1.0f; // East-west component
                windDir.y = (latitude > 0) ? -0.2f : 0.2f; // Slight north-south component
                break;
                
            case CirculationCellType::Polar:
                // Easterly winds in polar regions
                // Direction is opposite in each hemisphere
                windDir.x = (latitude > 0) ? -0.8f : 0.8f; // East-west component
                windDir.y = (latitude > 0) ? -0.4f : 0.4f; // North-south component
                break;
        }
        
        // Normalize wind direction
        if (glm::length(windDir) > 0.001f) {
            windDir = glm::normalize(windDir);
        }
        
        // Wind strength varies with latitude
        // Strongest at mid-latitudes, weaker at equator and poles
        float strength = 0.5f + 0.5f * std::sin(absLat * DEG_TO_RAD * 2.0f);
        
        // Adjust for planet rotation rate (faster rotation = stronger winds)
        strength *= (0.5f + std::min(1.0f, m_parameters.rotationRate));
        
        // Set wind vector
        m_windVectors[i] = windDir * strength;
    }
}

void AtmosphericCirculation::ApplyCoriolisEffect(std::vector<glm::vec3>& windVectors, float rotationRate)
{
    // The Coriolis effect deflects moving air to the right in the Northern Hemisphere
    // and to the left in the Southern Hemisphere
    
    // Coriolis parameter depends on rotation rate and latitude
    float coriolisStrength = rotationRate * 0.1f; // Scale factor
    
    // Apply to each point
    for (int i = 0; i < windVectors.size(); ++i) {
        // Convert grid index to latitude
        int y = (i / m_resolution) % m_resolution;
        float ny = (y / static_cast<float>(m_resolution)) * 2.0f - 1.0f;
        float latitude = ny * 90.0f;
        
        // Coriolis parameter (2Ω*sin(φ))
        float coriolisParameter = 2.0f * coriolisStrength * std::sin(latitude * DEG_TO_RAD);
        
        // Deflection angle depends on latitude and rotation rate
        float deflectionAngle = coriolisParameter * 10.0f; // Scale for visual effect
        
        // Limit maximum deflection
        deflectionAngle = std::max(-30.0f, std::min(30.0f, deflectionAngle));
        
        // Convert to radians
        deflectionAngle *= DEG_TO_RAD;
        
        // Rotate wind vector
        glm::vec3 originalWind = windVectors[i];
        float windSpeed = glm::length(originalWind);
        
        if (windSpeed > 0.001f) {
            // Normalize
            glm::vec3 windDir = originalWind / windSpeed;
            
            // Rotate in 2D (x,y plane)
            float cosA = std::cos(deflectionAngle);
            float sinA = std::sin(deflectionAngle);
            glm::vec3 rotatedWind(
                windDir.x * cosA - windDir.y * sinA,
                windDir.x * sinA + windDir.y * cosA,
                windDir.z
            );
            
            // Apply rotated direction with original speed
            windVectors[i] = rotatedWind * windSpeed;
        }
    }
}

void AtmosphericCirculation::ApplyTopographicalEffects(std::vector<glm::vec3>& windVectors, 
                                                     const std::vector<float>& elevationData)
{
    // Topography affects wind patterns by:
    // 1. Blocking or channeling winds
    // 2. Creating upslope/downslope winds
    // 3. Creating rain shadows
    
    // Apply to each point
    for (int i = 0; i < windVectors.size(); ++i) {
        // Skip if no elevation data
        if (i >= elevationData.size()) continue;
        
        float elevation = elevationData[i];
        
        // Get neighboring points to calculate slope
        // This is a simplified approach - a real implementation would use proper neighbors
        int x = i % m_resolution;
        int y = (i / m_resolution) % m_resolution;
        int face = i / (m_resolution * m_resolution);
        
        // Check east neighbor
        int eastIdx = ((x + 1) % m_resolution) + y * m_resolution + face * m_resolution * m_resolution;
        float eastElev = (eastIdx < elevationData.size()) ? elevationData[eastIdx] : elevation;
        
        // Check north neighbor
        int northIdx = x + ((y + 1) % m_resolution) * m_resolution + face * m_resolution * m_resolution;
        float northElev = (northIdx < elevationData.size()) ? elevationData[northIdx] : elevation;
        
        // Calculate slope components
        float eastSlope = eastElev - elevation;
        float northSlope = northElev - elevation;
        
        // Create slope vector
        glm::vec3 slopeVector(eastSlope, northSlope, 0.0f);
        float slopeMagnitude = glm::length(slopeVector);
        
        // Only apply if slope is significant
        if (slopeMagnitude > 0.05f) {
            // Normalize slope vector
            slopeVector /= slopeMagnitude;
            
            // Wind tends to be deflected along slopes
            glm::vec3 originalWind = windVectors[i];
            float windSpeed = glm::length(originalWind);
            
            if (windSpeed > 0.001f) {
                // Calculate deflection
                float deflectionStrength = slopeMagnitude * 0.5f; // Scale factor
                deflectionStrength = std::min(0.8f, deflectionStrength); // Limit maximum effect
                
                // Blend original wind with slope-deflected wind
                glm::vec3 deflectedWind = originalWind * (1.0f - deflectionStrength) - 
                                         slopeVector * deflectionStrength * windSpeed;
                
                // Reduce wind speed over high elevations and rough terrain
                float speedReduction = 1.0f - std::min(0.5f, elevation * 0.5f + slopeMagnitude * 0.3f);
                
                // Apply modified wind
                windVectors[i] = deflectedWind * speedReduction;
            }
        }
    }
}

void AtmosphericCirculation::GeneratePressureSystems(int resolution)
{
    // Generate global pressure patterns
    // Pressure is generally higher at subtropical latitudes (30°N/S) and polar regions
    // and lower at the equator and subpolar regions (60°N/S)
    
    // Iterate through the grid
    for (int i = 0; i < m_pressureGrid.size(); ++i) {
        // Convert grid index to latitude
        int y = (i / resolution) % resolution;
        float ny = (y / static_cast<float>(resolution)) * 2.0f - 1.0f;
        float latitude = ny * 90.0f;
        
        // Base pressure pattern based on latitude
        m_pressureGrid[i] = CalculatePressureGradient(latitude);
        
        // Add some random variation
        // In a real implementation, this would be coherent noise
        float variation = (std::sin(i * 0.1f) + std::cos(i * 0.2f)) * 0.05f;
        m_pressureGrid[i] += variation;
        
        // Clamp to valid range
        m_pressureGrid[i] = std::max(0.0f, std::min(1.0f, m_pressureGrid[i]));
    }
}

void AtmosphericCirculation::GenerateTemperatureMap(int resolution)
{
    // Generate global temperature patterns
    // Temperature is primarily determined by latitude, with equator being warmest
    // and poles being coldest
    
    // Iterate through the grid
    for (int i = 0; i < m_temperatureGrid.size(); ++i) {
        // Convert grid index to latitude
        int y = (i / resolution) % resolution;
        float ny = (y / static_cast<float>(resolution)) * 2.0f - 1.0f;
        float latitude = ny * 90.0f;
        
        // Base temperature based on latitude
        m_temperatureGrid[i] = CalculateBaseTemperature(latitude);
        
        // Add some random variation
        // In a real implementation, this would be coherent noise
        float variation = (std::sin(i * 0.1f) + std::cos(i * 0.2f)) * 2.0f;
        m_temperatureGrid[i] += variation;
    }
}

void AtmosphericCirculation::GenerateHadleyCell(float planetRadius, float rotationRate)
{
    // The Hadley cell is the tropical atmospheric circulation cell
    // It involves rising air at the equator, poleward flow aloft,
    // sinking air at around 30° latitude, and equatorward flow near the surface
    
    // Strength of the Hadley cell depends on planet size and rotation
    float cellStrength = planetRadius * 0.01f; // Scale with planet size
    
    // Faster rotation makes cells narrower but more intense
    float latitudinalExtent = 30.0f * (1.0f / (0.5f + rotationRate * 0.5f));
    latitudinalExtent = std::max(15.0f, std::min(45.0f, latitudinalExtent));
    
    // This would be used to adjust wind patterns in the Hadley cell region
    // In a full implementation, this would modify the wind vectors directly
}

void AtmosphericCirculation::GenerateFerrelCell(float planetRadius, float rotationRate)
{
    // The Ferrel cell is the mid-latitude atmospheric circulation cell
    // It involves rising air at around 60° latitude, equatorward flow aloft,
    // sinking air at around 30° latitude, and poleward flow near the surface
    
    // Strength of the Ferrel cell depends on planet size and rotation
    float cellStrength = planetRadius * 0.005f; // Weaker than Hadley cell
    
    // Faster rotation makes cells narrower but more intense
    float latitudinalExtent = 30.0f * (1.0f / (0.5f + rotationRate * 0.5f));
    latitudinalExtent = std::max(15.0f, std::min(45.0f, latitudinalExtent));
    
    // This would be used to adjust wind patterns in the Ferrel cell region
    // In a full implementation, this would modify the wind vectors directly
}

void AtmosphericCirculation::GeneratePolarCell(float planetRadius, float rotationRate)
{
    // The Polar cell is the high-latitude atmospheric circulation cell
    // It involves rising air at around 60° latitude, poleward flow aloft,
    // sinking air at the poles, and equatorward flow near the surface
    
    // Strength of the Polar cell depends on planet size and rotation
    float cellStrength = planetRadius * 0.007f; // Intermediate strength
    
    // Faster rotation makes cells narrower but more intense
    float latitudinalExtent = 30.0f * (1.0f / (0.5f + rotationRate * 0.5f));
    latitudinalExtent = std::max(15.0f, std::min(45.0f, latitudinalExtent));
    
    // This would be used to adjust wind patterns in the Polar cell region
    // In a full implementation, this would modify the wind vectors directly
}

int AtmosphericCirculation::GetGridIndex(float latitude, float longitude) const
{
    if (m_resolution == 0) return -1;
    
    // Convert latitude/longitude to grid coordinates
    // This is a simplified conversion - a real implementation would use proper mapping
    
    // Normalize latitude and longitude to 0-1 range
    float normLat = (latitude + 90.0f) / 180.0f;
    float normLon = (longitude + 180.0f) / 360.0f;
    
    // Convert to grid coordinates
    int x = static_cast<int>(normLon * m_resolution);
    int y = static_cast<int>(normLat * m_resolution);
    
    // Determine cube face (simplified)
    int face = 0;
    if (longitude >= -180.0f && longitude < -90.0f) face = 0;
    else if (longitude >= -90.0f && longitude < 0.0f) face = 1;
    else if (longitude >= 0.0f && longitude < 90.0f) face = 2;
    else face = 3;
    
    // Special case for poles
    if (latitude > 80.0f) face = 4;
    else if (latitude < -80.0f) face = 5;
    
    // Calculate index
    int index = x + y * m_resolution + face * m_resolution * m_resolution;
    
    // Bounds check
    if (index >= 0 && index < m_windVectors.size()) {
        return index;
    }
    
    return -1;
}

float AtmosphericCirculation::CalculateBaseTemperature(float latitude) const
{
    // Calculate base temperature based on latitude
    // This is a simplified model - a real implementation would be more sophisticated
    
    // Convert latitude to radians
    float latRad = std::abs(latitude) * DEG_TO_RAD;
    
    // Temperature decreases from equator to poles
    // Using a cosine function to model this
    float tempFactor = std::cos(latRad);
    
    // Scale to temperature range
    // For Earth-like planets: equator ~30°C, poles ~-30°C
    float baseTemp = tempFactor * 30.0f;
    
    // Adjust based on planet's distance from star
    // Closer to star = hotter, further = colder
    float distanceFactor = 1.0f / (m_parameters.semiMajorAxis * 0.5f + 0.5f);
    baseTemp *= distanceFactor;
    
    // Adjust based on atmosphere density
    // Thicker atmosphere = more greenhouse effect = warmer
    float atmosphereFactor = 0.8f + m_parameters.atmosphereDensity * 0.4f;
    baseTemp *= atmosphereFactor;
    
    return baseTemp;
}

float AtmosphericCirculation::CalculatePressureGradient(float latitude) const
{
    // Calculate pressure gradient based on latitude
    // This is a simplified model - a real implementation would be more sophisticated
    
    // Convert latitude to radians
    float latRad = latitude * DEG_TO_RAD;
    
    // Pressure pattern: high at subtropical highs (30°) and poles, low at equator and subpolar lows (60°)
    // Using a combination of cosine functions to model this
    float pressureFactor = 0.5f + 0.25f * std::cos(latRad * 6.0f) + 0.25f * std::cos(latRad * 2.0f);
    
    // Normalize to 0-1 range
    pressureFactor = std::max(0.0f, std::min(1.0f, pressureFactor));
    
    return pressureFactor;
}

} // namespace Core
} // namespace WorldGen
