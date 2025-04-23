#include "WorldGen/Renderer/BiomeVisualizer.h"
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <glm/gtc/noise.hpp>

namespace WorldGen {
namespace Renderer {

BiomeVisualizer::BiomeVisualizer(GlobeRenderer& renderer)
    : m_renderer(renderer),
      m_season(0.0f),
      m_snowVisualizationEnabled(true),
      m_glacierVisualizationEnabled(true),
      m_detailLevel(1.0f)
{
    InitializeColorConfig();
}

BiomeVisualizer::~BiomeVisualizer()
{
}

void BiomeVisualizer::InitializeColorConfig()
{
    using BiomeType = Core::Biomes::BiomeType;
    
    // Forest Biomes
    m_biomeColors[BiomeType::TropicalRainforest] = BiomeColorConfig(
        glm::vec3(0.0f, 0.4f, 0.0f),    // Dark green
        glm::vec3(0.0f, 0.1f, 0.0f),    // Slight variation
        0.9f,                           // High roughness
        0.1f                            // Low specular
    );
    
    m_biomeColors[BiomeType::TropicalSeasonalForest] = BiomeColorConfig(
        glm::vec3(0.1f, 0.5f, 0.1f),    // Medium green
        glm::vec3(0.1f, 0.1f, 0.0f),    // Yellow-green variation
        0.8f,                           // High roughness
        0.15f                           // Low specular
    );
    
    m_biomeColors[BiomeType::TemperateDeciduousForest] = BiomeColorConfig(
        glm::vec3(0.2f, 0.5f, 0.1f),    // Light green
        glm::vec3(0.1f, 0.1f, 0.0f),    // Yellow-green variation
        0.7f,                           // Medium roughness
        0.2f                            // Medium specular
    );
    
    m_biomeColors[BiomeType::TemperateRainforest] = BiomeColorConfig(
        glm::vec3(0.1f, 0.4f, 0.2f),    // Blue-green
        glm::vec3(0.0f, 0.1f, 0.1f),    // Slight variation
        0.8f,                           // High roughness
        0.15f                           // Low specular
    );
    
    m_biomeColors[BiomeType::BorealForest] = BiomeColorConfig(
        glm::vec3(0.1f, 0.3f, 0.1f),    // Dark blue-green
        glm::vec3(0.0f, 0.05f, 0.05f),  // Slight variation
        0.7f,                           // Medium roughness
        0.2f                            // Medium specular
    );
    
    m_biomeColors[BiomeType::MontaneForest] = BiomeColorConfig(
        glm::vec3(0.2f, 0.4f, 0.2f),    // Medium green
        glm::vec3(0.1f, 0.1f, 0.0f),    // Yellow-green variation
        0.8f,                           // High roughness
        0.15f                           // Low specular
    );
    
    // Grassland Biomes
    m_biomeColors[BiomeType::TropicalSavanna] = BiomeColorConfig(
        glm::vec3(0.7f, 0.7f, 0.3f),    // Yellow-green
        glm::vec3(0.1f, 0.1f, 0.0f),    // Slight variation
        0.6f,                           // Medium roughness
        0.3f                            // Medium specular
    );
    
    m_biomeColors[BiomeType::TemperateGrassland] = BiomeColorConfig(
        glm::vec3(0.8f, 0.8f, 0.4f),    // Light yellow-green
        glm::vec3(0.1f, 0.1f, 0.0f),    // Slight variation
        0.5f,                           // Medium roughness
        0.3f                            // Medium specular
    );
    
    m_biomeColors[BiomeType::AlpineGrassland] = BiomeColorConfig(
        glm::vec3(0.6f, 0.7f, 0.3f),    // Green-yellow
        glm::vec3(0.1f, 0.1f, 0.0f),    // Slight variation
        0.6f,                           // Medium roughness
        0.25f                           // Medium specular
    );
    
    // Desert and Xeric Biomes
    m_biomeColors[BiomeType::HotDesert] = BiomeColorConfig(
        glm::vec3(0.9f, 0.8f, 0.5f),    // Sand color
        glm::vec3(0.1f, 0.1f, 0.1f),    // Moderate variation
        0.4f,                           // Low roughness
        0.4f                            // High specular
    );
    
    m_biomeColors[BiomeType::ColdDesert] = BiomeColorConfig(
        glm::vec3(0.8f, 0.7f, 0.6f),    // Light brown
        glm::vec3(0.1f, 0.1f, 0.1f),    // Moderate variation
        0.5f,                           // Medium roughness
        0.3f                            // Medium specular
    );
    
    m_biomeColors[BiomeType::SemiDesert] = BiomeColorConfig(
        glm::vec3(0.8f, 0.7f, 0.4f),    // Tan
        glm::vec3(0.1f, 0.1f, 0.0f),    // Slight variation
        0.5f,                           // Medium roughness
        0.35f                           // Medium-high specular
    );
    
    m_biomeColors[BiomeType::XericShrubland] = BiomeColorConfig(
        glm::vec3(0.7f, 0.6f, 0.3f),    // Olive
        glm::vec3(0.1f, 0.1f, 0.0f),    // Slight variation
        0.6f,                           // Medium roughness
        0.3f                            // Medium specular
    );
    
    // Tundra and Cold Biomes
    m_biomeColors[BiomeType::ArcticTundra] = BiomeColorConfig(
        glm::vec3(0.7f, 0.7f, 0.7f),    // Light gray
        glm::vec3(0.1f, 0.1f, 0.1f),    // Moderate variation
        0.6f,                           // Medium roughness
        0.25f                           // Medium specular
    );
    
    m_biomeColors[BiomeType::AlpineTundra] = BiomeColorConfig(
        glm::vec3(0.6f, 0.6f, 0.6f),    // Medium gray
        glm::vec3(0.1f, 0.1f, 0.1f),    // Moderate variation
        0.7f,                           // Medium-high roughness
        0.2f                            // Low-medium specular
    );
    
    m_biomeColors[BiomeType::PolarDesert] = BiomeColorConfig(
        glm::vec3(0.8f, 0.8f, 0.8f),    // Very light gray
        glm::vec3(0.05f, 0.05f, 0.05f), // Slight variation
        0.5f,                           // Medium roughness
        0.3f                            // Medium specular
    );
    
    // Wetland Biomes
    m_biomeColors[BiomeType::TemperateWetland] = BiomeColorConfig(
        glm::vec3(0.2f, 0.4f, 0.3f),    // Blue-green
        glm::vec3(0.05f, 0.1f, 0.05f),  // Slight variation
        0.7f,                           // Medium-high roughness
        0.3f                            // Medium specular
    );
    
    m_biomeColors[BiomeType::TropicalWetland] = BiomeColorConfig(
        glm::vec3(0.1f, 0.5f, 0.3f),    // Green-blue
        glm::vec3(0.05f, 0.1f, 0.05f),  // Slight variation
        0.8f,                           // High roughness
        0.2f                            // Low-medium specular
    );
}

void BiomeVisualizer::UpdateVisualization(
    const std::vector<Core::Biomes::BiomeData>& biomeData,
    const std::vector<Core::Climate::SnowData>& snowData,
    int resolution,
    float season)
{
    m_season = season;
    
    // Create color buffer for the renderer
    std::vector<glm::vec3> colorBuffer(biomeData.size());
    std::vector<float> roughnessBuffer(biomeData.size());
    std::vector<float> specularBuffer(biomeData.size());
    
    // Calculate colors for each tile
    for (size_t i = 0; i < biomeData.size(); ++i) {
        // Calculate 2D coordinates for noise generation
        int x = i % resolution;
        int y = i / resolution;
        
        // Generate noise for variation
        float noiseValue = GenerateNoise(static_cast<float>(x), static_cast<float>(y), 0.1f * m_detailLevel);
        
        // Calculate final color
        colorBuffer[i] = CalculateTileColor(biomeData[i], snowData[i], noiseValue, m_season);
        
        // Get base biome properties
        const auto& primaryBiomeConfig = m_biomeColors[biomeData[i].primaryBiome];
        const auto& secondaryBiomeConfig = m_biomeColors[biomeData[i].secondaryBiome];
        
        // Blend roughness and specular values
        roughnessBuffer[i] = primaryBiomeConfig.roughness * (1.0f - biomeData[i].transitionFactor) +
                             secondaryBiomeConfig.roughness * biomeData[i].transitionFactor;
        
        specularBuffer[i] = primaryBiomeConfig.specular * (1.0f - biomeData[i].transitionFactor) +
                            secondaryBiomeConfig.specular * biomeData[i].transitionFactor;
        
        // Adjust for snow
        if (m_snowVisualizationEnabled && snowData[i].hasSeasonalSnow) {
            // Calculate snow coverage based on season
            float snowCoverage = CalculateSnowCoverage(snowData[i], m_season);
            
            // Snow increases roughness and reduces specular
            roughnessBuffer[i] = roughnessBuffer[i] * (1.0f - snowCoverage) + 0.8f * snowCoverage;
            specularBuffer[i] = specularBuffer[i] * (1.0f - snowCoverage) + 0.2f * snowCoverage;
        }
        
        // Adjust for glaciers
        if (m_glacierVisualizationEnabled && snowData[i].isGlacier) {
            // Glaciers are smoother and more reflective than snow
            roughnessBuffer[i] = 0.3f;
            specularBuffer[i] = 0.6f;
        }
    }
    
    // Update the renderer with new color data
    m_renderer.UpdateColorData(colorBuffer, roughnessBuffer, specularBuffer);
}

void BiomeVisualizer::SetSeason(float season)
{
    m_season = std::max(0.0f, std::min(1.0f, season));
}

void BiomeVisualizer::SetSnowVisualizationEnabled(bool enabled)
{
    m_snowVisualizationEnabled = enabled;
}

void BiomeVisualizer::SetGlacierVisualizationEnabled(bool enabled)
{
    m_glacierVisualizationEnabled = enabled;
}

void BiomeVisualizer::SetDetailLevel(float detailLevel)
{
    m_detailLevel = std::max(0.1f, std::min(1.0f, detailLevel));
}

std::vector<std::pair<Core::Biomes::BiomeType, glm::vec3>> BiomeVisualizer::GenerateBiomeLegend() const
{
    std::vector<std::pair<Core::Biomes::BiomeType, glm::vec3>> legend;
    
    for (const auto& entry : m_biomeColors) {
        legend.emplace_back(entry.first, entry.second.baseColor);
    }
    
    return legend;
}

glm::vec3 BiomeVisualizer::CalculateTileColor(
    const Core::Biomes::BiomeData& biomeData,
    const Core::Climate::SnowData& snowData,
    float noiseValue,
    float season)
{
    // Get color configurations for primary and secondary biomes
    const auto& primaryBiomeConfig = m_biomeColors[biomeData.primaryBiome];
    const auto& secondaryBiomeConfig = m_biomeColors[biomeData.secondaryBiome];
    
    // Apply noise variation to base colors
    glm::vec3 primaryColor = primaryBiomeConfig.baseColor + 
                            (noiseValue - 0.5f) * 2.0f * primaryBiomeConfig.variationColor;
    
    glm::vec3 secondaryColor = secondaryBiomeConfig.baseColor + 
                              (noiseValue - 0.5f) * 2.0f * secondaryBiomeConfig.variationColor;
    
    // Apply seasonal adjustments
    primaryColor = CalculateSeasonalColor(biomeData.primaryBiome, primaryColor, season);
    secondaryColor = CalculateSeasonalColor(biomeData.secondaryBiome, secondaryColor, season);
    
    // Blend between primary and secondary biome colors
    glm::vec3 blendedColor = BlendColors(primaryColor, secondaryColor, biomeData.transitionFactor);
    
    // Apply vegetation density adjustment
    blendedColor = AdjustForVegetationDensity(blendedColor, biomeData.vegetationDensity);
    
    // Apply snow overlay if enabled
    if (m_snowVisualizationEnabled && snowData.hasSeasonalSnow) {
        // Calculate snow coverage based on season
        float snowCoverage = CalculateSnowCoverage(snowData, season);
        
        // Blend with snow color
        glm::vec3 snowColor(1.0f, 1.0f, 1.0f);  // Pure white
        blendedColor = BlendColors(blendedColor, snowColor, snowCoverage);
    }
    
    // Apply glacier overlay if enabled
    if (m_glacierVisualizationEnabled && snowData.isGlacier) {
        // Glacier color (slight blue tint)
        glm::vec3 glacierColor(0.8f, 0.9f, 1.0f);
        blendedColor = BlendColors(blendedColor, glacierColor, 0.9f);  // Strong blend
    }
    
    return blendedColor;
}

glm::vec3 BiomeVisualizer::CalculateSeasonalColor(
    Core::Biomes::BiomeType biomeType,
    const glm::vec3& baseColor,
    float season)
{
    using BiomeType = Core::Biomes::BiomeType;
    
    // Adjust color based on season and biome type
    glm::vec3 adjustedColor = baseColor;
    
    // Season is 0.0 (winter start) to 1.0 (full year cycle)
    // Convert to 0.0 (winter) - 0.25 (spring) - 0.5 (summer) - 0.75 (fall) - 1.0 (winter)
    float seasonCycle = season * 2.0f * glm::pi<float>();
    
    switch (biomeType) {
        case BiomeType::TemperateDeciduousForest:
            // Spring: more green
            // Summer: deep green
            // Fall: orange/red
            // Winter: brown
            if (season < 0.25f) {  // Winter to Spring
                float t = season / 0.25f;
                adjustedColor = BlendColors(glm::vec3(0.4f, 0.3f, 0.2f), glm::vec3(0.3f, 0.6f, 0.2f), t);
            } else if (season < 0.5f) {  // Spring to Summer
                float t = (season - 0.25f) / 0.25f;
                adjustedColor = BlendColors(glm::vec3(0.3f, 0.6f, 0.2f), glm::vec3(0.2f, 0.5f, 0.1f), t);
            } else if (season < 0.75f) {  // Summer to Fall
                float t = (season - 0.5f) / 0.25f;
                adjustedColor = BlendColors(glm::vec3(0.2f, 0.5f, 0.1f), glm::vec3(0.7f, 0.4f, 0.1f), t);
            } else {  // Fall to Winter
                float t = (season - 0.75f) / 0.25f;
                adjustedColor = BlendColors(glm::vec3(0.7f, 0.4f, 0.1f), glm::vec3(0.4f, 0.3f, 0.2f), t);
            }
            break;
            
        case BiomeType::BorealForest:
            // Less seasonal variation, but some
            if (season < 0.5f) {  // Winter to Summer
                float t = season / 0.5f;
                adjustedColor = BlendColors(glm::vec3(0.1f, 0.25f, 0.1f), glm::vec3(0.1f, 0.35f, 0.1f), t);
            } else {  // Summer to Winter
                float t = (season - 0.5f) / 0.5f;
                adjustedColor = BlendColors(glm::vec3(0.1f, 0.35f, 0.1f), glm::vec3(0.1f, 0.25f, 0.1f), t);
            }
            break;
            
        case BiomeType::TemperateGrassland:
            // Green in spring/summer, yellow/brown in fall/winter
            if (season < 0.25f) {  // Winter to Spring
                float t = season / 0.25f;
                adjustedColor = BlendColors(glm::vec3(0.7f, 0.6f, 0.3f), glm::vec3(0.6f, 0.8f, 0.3f), t);
            } else if (season < 0.5f) {  // Spring to Summer
                float t = (season - 0.25f) / 0.25f;
                adjustedColor = BlendColors(glm::vec3(0.6f, 0.8f, 0.3f), glm::vec3(0.8f, 0.8f, 0.4f), t);
            } else if (season < 0.75f) {  // Summer to Fall
                float t = (season - 0.5f) / 0.25f;
                adjustedColor = BlendColors(glm::vec3(0.8f, 0.8f, 0.4f), glm::vec3(0.8f, 0.7f, 0.3f), t);
            } else {  // Fall to Winter
                float t = (season - 0.75f) / 0.25f;
                adjustedColor = BlendColors(glm::vec3(0.8f, 0.7f, 0.3f), glm::vec3(0.7f, 0.6f, 0.3f), t);
            }
            break;
            
        case BiomeType::TropicalSavanna:
            // Wet season (greener) and dry season (browner)
            adjustedColor = BlendColors(
                glm::vec3(0.7f, 0.7f, 0.3f),  // Dry season
                glm::vec3(0.5f, 0.7f, 0.2f),  // Wet season
                0.5f + 0.5f * std::sin(seasonCycle)
            );
            break;
            
        case BiomeType::TropicalSeasonalForest:
            // Wet season (greener) and dry season (less green)
            adjustedColor = BlendColors(
                glm::vec3(0.2f, 0.5f, 0.1f),  // Dry season
                glm::vec3(0.1f, 0.6f, 0.1f),  // Wet season
                0.5f + 0.5f * std::sin(seasonCycle)
            );
            break;
            
        // Biomes with minimal seasonal color change
        case BiomeType::TropicalRainforest:
        case BiomeType::HotDesert:
        case BiomeType::ColdDesert:
        case BiomeType::PolarDesert:
            // No significant seasonal changes
            break;
            
        default:
            // Apply a subtle seasonal variation to other biomes
            adjustedColor = BlendColors(
                baseColor,
                baseColor * glm::vec3(1.1f, 1.1f, 0.9f),  // Slightly lighter/yellower
                0.5f + 0.2f * std::sin(seasonCycle)
            );
            break;
    }
    
    return adjustedColor;
}

float BiomeVisualizer::CalculateSnowCoverage(
    const Core::Climate::SnowData& snowData,
    float season)
{
    // No snow
    if (!snowData.hasSeasonalSnow) {
        return 0.0f;
    }
    
    // Permanent snow or glacier is always fully covered
    if (snowData.isPermanentSnow || snowData.isGlacier) {
        return 1.0f;
    }
    
    // For seasonal snow, calculate coverage based on season
    // Season is 0.0 (winter start) to 1.0 (full year cycle)
    
    // Convert season to a position in the year (0.0 = winter, 0.5 = summer)
    float yearPosition = season;
    if (yearPosition > 0.5f) {
        yearPosition = 1.0f - yearPosition;
    }
    yearPosition = yearPosition * 2.0f;  // Scale to 0.0-1.0 range
    
    // Calculate snow coverage based on snow months and current season
    float snowMonthsFraction = static_cast<float>(snowData.snowMonthsPerYear) / 12.0f;
    
    // More snow months means snow persists longer into spring/starts earlier in fall
    float snowThreshold = 1.0f - snowMonthsFraction;
    
    // Calculate coverage
    if (yearPosition < snowThreshold) {
        return 0.0f;  // No snow in summer
    } else {
        // Gradual increase/decrease at season boundaries
        float t = (yearPosition - snowThreshold) / (1.0f - snowThreshold);
        return std::min(1.0f, t * 2.0f);  // Faster accumulation
    }
}

glm::vec3 BiomeVisualizer::AdjustForVegetationDensity(
    const glm::vec3& baseColor,
    float vegetationDensity)
{
    // For low vegetation density, blend toward a more soil-like color
    glm::vec3 soilColor(0.6f, 0.5f, 0.4f);  // Brown soil color
    
    // Adjust blend factor based on vegetation density
    float blendFactor = std::max(0.0f, vegetationDensity);
    
    return BlendColors(soilColor, baseColor, blendFactor);
}

float BiomeVisualizer::GenerateNoise(float x, float y, float scale)
{
    // Use simplex noise for natural-looking variation
    return glm::simplex(glm::vec2(x * scale, y * scale)) * 0.5f + 0.5f;
}

glm::vec3 BiomeVisualizer::BlendColors(const glm::vec3& color1, const glm::vec3& color2, float factor)
{
    factor = std::max(0.0f, std::min(1.0f, factor));
    return color1 * (1.0f - factor) + color2 * factor;
}

} // namespace Renderer
} // namespace WorldGen
