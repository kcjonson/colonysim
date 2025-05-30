#pragma once

#include <vector>
#include <memory>
#include "../Core/TerrainTypes.h"

namespace WorldGen {

// Forward declarations
class ProgressTracker;

namespace Generators {

// Forward declarations
class World;

/**
 * @brief Generate biomes and terrain types based on environmental factors
 * 
 * This function analyzes elevation, temperature, moisture, and other factors
 * to assign appropriate terrain types and biomes to each tile.
 * 
 * @param world The world to generate biomes for
 * @param progressTracker Optional progress tracker for UI updates
 */
void GenerateBiomes(World* world, std::shared_ptr<ProgressTracker> progressTracker = nullptr);

/**
 * @brief Determine terrain type based on elevation
 * 
 * @param elevation The tile's elevation (0.0 to 1.0)
 * @param waterLevel The water level threshold
 * @return The appropriate terrain type
 */
TerrainType DetermineTerrainType(float elevation, float waterLevel = 0.4f);

/**
 * @brief Determine biome type based on environmental factors
 * 
 * @param elevation The tile's elevation (0.0 to 1.0)
 * @param temperature The tile's temperature (0.0 to 1.0)
 * @param moisture The tile's moisture (0.0 to 1.0)
 * @param terrainType The tile's terrain type
 * @return The appropriate biome type
 */
BiomeType DetermineBiomeType(float elevation, float temperature, float moisture, TerrainType terrainType);

} // namespace Generators
} // namespace WorldGen