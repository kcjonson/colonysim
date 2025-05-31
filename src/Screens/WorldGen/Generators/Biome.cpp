#include "Biome.h"
#include "World.h"
#include "Tile.h"
#include "../ProgressTracker.h"
#include "../Core/WorldGenParameters.h"
#include <iostream>
#include <unordered_map>

namespace WorldGen {
namespace Generators {

TerrainType DetermineTerrainType(float elevation, float waterLevel) {
    // Determine terrain type based on elevation in meters from planet center
    // waterLevel = planet radius (sea level reference)
    if (elevation < waterLevel - 1000.0f) {
        return TerrainType::Ocean;      // Deep ocean (>1000m below sea level)
    } else if (elevation < waterLevel - 50.0f) {
        return TerrainType::Shallow;    // Shallow water (50-1000m below sea level)
    } else if (elevation < waterLevel + 50.0f) {
        return TerrainType::Beach;      // Beach/coastal (±50m of sea level)
    } else if (elevation < waterLevel + 1000.0f) {
        return TerrainType::Lowland;    // Lowlands (0-1000m above sea level)
    } else if (elevation < waterLevel + 2000.0f) {
        return TerrainType::Highland;   // Highlands (1000-2000m above sea level)
    } else if (elevation < waterLevel + 4000.0f) {
        return TerrainType::Mountain;   // Mountains (2000-4000m above sea level)
    } else {
        return TerrainType::Peak;       // High peaks (>4000m above sea level)
    }
}

BiomeType DetermineBiomeType(float elevation, float temperature, float moisture, TerrainType terrainType) {
    // Water biomes
    if (terrainType == TerrainType::Ocean) {
        return BiomeType::DeepOcean;
    } else if (terrainType == TerrainType::Shallow) {
        if (temperature > 0.8f && moisture > 0.7f) {
            return BiomeType::Reef; // Coral reefs in warm shallow water
        }
        return BiomeType::Ocean;
    }
    
    // Land biomes based on temperature and moisture
    // Using a simplified Whittaker biome classification
    
    // Very cold regions (tundra/ice)
    if (temperature < 0.2f) {
        if (elevation > 0.8f) {
            return BiomeType::AlpineTundra;
        } else if (moisture < 0.2f) {
            return BiomeType::PolarDesert;
        } else {
            return BiomeType::ArcticTundra;
        }
    }
    
    // Cold regions (boreal/taiga)
    if (temperature < 0.4f) {
        if (moisture > 0.4f) {
            return BiomeType::BorealForest;
        } else {
            return BiomeType::ColdDesert;
        }
    }
    
    // Temperate regions
    if (temperature < 0.6f) {
        if (moisture > 0.7f) {
            return BiomeType::TemperateRainforest;
        } else if (moisture > 0.4f) {
            return BiomeType::TemperateDeciduousForest;
        } else if (moisture > 0.2f) {
            return BiomeType::TemperateGrassland;
        } else {
            return BiomeType::XericShrubland;
        }
    }
    
    // Warm/subtropical regions
    if (temperature < 0.8f) {
        if (moisture > 0.6f) {
            return BiomeType::TropicalSeasonalForest;
        } else if (moisture > 0.3f) {
            return BiomeType::TropicalSavanna;
        } else if (moisture > 0.1f) {
            return BiomeType::SemiDesert;
        } else {
            return BiomeType::HotDesert;
        }
    }
    
    // Tropical regions
    if (moisture > 0.7f) {
        return BiomeType::TropicalRainforest;
    } else if (moisture > 0.4f) {
        return BiomeType::TropicalSeasonalForest;
    } else if (moisture > 0.2f) {
        return BiomeType::TropicalSavanna;
    } else {
        return BiomeType::HotDesert;
    }
}

void GenerateBiomes(World* world, std::shared_ptr<ProgressTracker> progressTracker) {
    if (!world) {
        std::cerr << "Error: Invalid world for biome generation" << std::endl;
        return;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(0.0f, "Generating biomes...");
    }
    
    const auto& tiles = world->GetTiles();
    // Use planet's physical radius as sea level reference
    const float waterLevel = PlanetParameters().physicalRadiusMeters;
    
    std::cout << "Generating biomes for " << tiles.size() << " tiles..." << std::endl;
    
    // Counters for statistics
    std::unordered_map<TerrainType, int> terrainCounts;
    std::unordered_map<BiomeType, int> biomeCounts;
    
    // Process each tile
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        
        // Get environmental factors
        float elevation = tile.GetElevation();
        float temperature = tile.GetTemperature();
        float moisture = tile.GetMoisture();
        
        // Determine terrain type based on elevation
        TerrainType terrainType = DetermineTerrainType(elevation, waterLevel);
        const_cast<Tile&>(tile).SetTerrainType(terrainType);
        terrainCounts[terrainType]++;
        
        // Determine biome type based on all factors
        BiomeType biomeType = DetermineBiomeType(elevation, temperature, moisture, terrainType);
        const_cast<Tile&>(tile).SetBiomeType(biomeType);
        biomeCounts[biomeType]++;
        
        // Report progress periodically
        if (progressTracker && i % 1000 == 0) {
            float progress = static_cast<float>(i) / tiles.size();
            std::string message = "Assigning biomes: " + std::to_string(i) + "/" + std::to_string(tiles.size());
            progressTracker->UpdateProgress(progress, message);
        }
    }
    
    // Log terrain distribution
    std::cout << "\n============ TERRAIN TYPE DISTRIBUTION ============" << std::endl;
    std::cout << "Ocean: " << terrainCounts[TerrainType::Ocean] << " tiles" << std::endl;
    std::cout << "Shallow: " << terrainCounts[TerrainType::Shallow] << " tiles" << std::endl;
    std::cout << "Beach: " << terrainCounts[TerrainType::Beach] << " tiles" << std::endl;
    std::cout << "Lowland: " << terrainCounts[TerrainType::Lowland] << " tiles" << std::endl;
    std::cout << "Highland: " << terrainCounts[TerrainType::Highland] << " tiles" << std::endl;
    std::cout << "Mountain: " << terrainCounts[TerrainType::Mountain] << " tiles" << std::endl;
    std::cout << "Peak: " << terrainCounts[TerrainType::Peak] << " tiles" << std::endl;
    std::cout << "Total: " << tiles.size() << " tiles" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    // Log biome distribution summary
    std::cout << "\n============ BIOME DISTRIBUTION ============" << std::endl;
    int landBiomes = 0, waterBiomes = 0;
    for (const auto& [biome, count] : biomeCounts) {
        if (count > 0) {
            // Check if it's a water biome
            if (biome == BiomeType::Ocean || 
                biome == BiomeType::DeepOcean || 
                biome == BiomeType::Reef) {
                waterBiomes += count;
            } else {
                landBiomes += count;
            }
        }
    }
    std::cout << "Land biomes: " << landBiomes << " tiles" << std::endl;
    std::cout << "Water biomes: " << waterBiomes << " tiles" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Biome generation complete!");
    }
    
    std::cout << "Biome generation complete." << std::endl;
}

} // namespace Generators
} // namespace WorldGen