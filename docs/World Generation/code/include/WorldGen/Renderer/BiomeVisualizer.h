#pragma once

/**
 * @file BiomeVisualizer.h
 * @brief Handles visualization of terrestrial biomes and snow/glacier features
 */

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "WorldGen/Core/Biomes/BiomeGenerator.h"
#include "WorldGen/Core/Climate/SnowAndGlacierCalculator.h"
#include "WorldGen/Renderer/GlobeRenderer.h"

namespace WorldGen {
namespace Renderer {

/**
 * @brief Color configuration for biome visualization
 */
struct BiomeColorConfig {
    glm::vec3 baseColor;       ///< Base color for the biome
    glm::vec3 variationColor;  ///< Color variation for noise
    float roughness;           ///< Surface roughness (0.0-1.0)
    float specular;            ///< Specular reflection (0.0-1.0)
    
    BiomeColorConfig() : 
        baseColor(0.5f, 0.5f, 0.5f),
        variationColor(0.1f, 0.1f, 0.1f),
        roughness(0.5f),
        specular(0.3f) {}
        
    BiomeColorConfig(const glm::vec3& base, const glm::vec3& variation, float rough, float spec) :
        baseColor(base),
        variationColor(variation),
        roughness(rough),
        specular(spec) {}
};

/**
 * @brief Handles visualization of biomes and snow/glacier features on the planet
 */
class BiomeVisualizer {
public:
    /**
     * @brief Constructor
     * @param renderer Reference to the globe renderer
     */
    explicit BiomeVisualizer(GlobeRenderer& renderer);
    
    /**
     * @brief Destructor
     */
    ~BiomeVisualizer();
    
    /**
     * @brief Initialize the biome color configuration
     */
    void InitializeColorConfig();
    
    /**
     * @brief Update the visualization based on biome and snow data
     * @param biomeData Vector of biome data for each tile
     * @param snowData Vector of snow data for each tile
     * @param resolution Resolution of the grid
     * @param season Current season (0.0-1.0, where 0.0 is winter start, 0.5 is summer start)
     */
    void UpdateVisualization(
        const std::vector<Core::Biomes::BiomeData>& biomeData,
        const std::vector<Core::Climate::SnowData>& snowData,
        int resolution,
        float season = 0.0f);
    
    /**
     * @brief Set the current season for visualization
     * @param season Season value (0.0-1.0, where 0.0 is winter start, 0.5 is summer start)
     */
    void SetSeason(float season);
    
    /**
     * @brief Toggle snow visualization
     * @param enabled Whether snow visualization is enabled
     */
    void SetSnowVisualizationEnabled(bool enabled);
    
    /**
     * @brief Toggle glacier visualization
     * @param enabled Whether glacier visualization is enabled
     */
    void SetGlacierVisualizationEnabled(bool enabled);
    
    /**
     * @brief Set the detail level for biome visualization
     * @param detailLevel Detail level (0.0-1.0)
     */
    void SetDetailLevel(float detailLevel);
    
    /**
     * @brief Generate a color legend for the current biome configuration
     * @return Vector of pairs containing biome type and color
     */
    std::vector<std::pair<Core::Biomes::BiomeType, glm::vec3>> GenerateBiomeLegend() const;

private:
    GlobeRenderer& m_renderer;
    std::unordered_map<Core::Biomes::BiomeType, BiomeColorConfig> m_biomeColors;
    float m_season;
    bool m_snowVisualizationEnabled;
    bool m_glacierVisualizationEnabled;
    float m_detailLevel;
    
    /**
     * @brief Calculate the final color for a tile based on biome and snow data
     * @param biomeData Biome data for the tile
     * @param snowData Snow data for the tile
     * @param noiseValue Noise value for variation (0.0-1.0)
     * @param season Current season (0.0-1.0)
     * @return Final color for the tile
     */
    glm::vec3 CalculateTileColor(
        const Core::Biomes::BiomeData& biomeData,
        const Core::Climate::SnowData& snowData,
        float noiseValue,
        float season);
    
    /**
     * @brief Calculate seasonal color variation for a biome
     * @param biomeType Biome type
     * @param baseColor Base color for the biome
     * @param season Current season (0.0-1.0)
     * @return Seasonally adjusted color
     */
    glm::vec3 CalculateSeasonalColor(
        Core::Biomes::BiomeType biomeType,
        const glm::vec3& baseColor,
        float season);
    
    /**
     * @brief Calculate snow overlay color based on snow data
     * @param snowData Snow data for the tile
     * @param season Current season (0.0-1.0)
     * @return Snow color overlay (white with appropriate alpha)
     */
    glm::vec4 CalculateSnowOverlay(
        const Core::Climate::SnowData& snowData,
        float season);
    
    /**
     * @brief Generate noise value for a specific position
     * @param x X coordinate
     * @param y Y coordinate
     * @param scale Noise scale
     * @return Noise value (0.0-1.0)
     */
    float GenerateNoise(float x, float y, float scale);
    
    /**
     * @brief Blend two colors based on a factor
     * @param color1 First color
     * @param color2 Second color
     * @param factor Blend factor (0.0-1.0)
     * @return Blended color
     */
    glm::vec3 BlendColors(const glm::vec3& color1, const glm::vec3& color2, float factor);
};

} // namespace Renderer
} // namespace WorldGen
