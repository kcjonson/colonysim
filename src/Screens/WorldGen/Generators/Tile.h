#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../Core/TerrainTypes.h" // Include TerrainTypes.h for TerrainType and BiomeType enums

namespace WorldGen {
namespace Generators {

/**
 * @brief Represents a single tile or region on the world.
 * 
 * This class stores information about a tile's center position, vertices,
 * edges, and its neighboring tiles in the world. It can represent either
 * a pentagon or a hexagon.
 */
class Tile {
public:    /**
     * @brief Enum representing the possible shapes of tiles.
     */
    enum class TileShape {
        Pentagon,
        Hexagon
    };    /**
     * @brief Construct a new Tile.
     * 
     * @param center The center position of the tile (normalized to unit sphere).
     * @param shape The shape of tile (pentagon or hexagon).
     */
    Tile(const glm::vec3& center, TileShape shape);

    /**
     * @brief Get the center position of the tile.
     * 
     * @return const glm::vec3& The center position (normalized to unit sphere).
     */
    const glm::vec3& GetCenter() const { return center; }    /**
     * @brief Get the shape of tile.
     * 
     * @return TileShape The tile shape (Pentagon or Hexagon).
     */
    TileShape GetShape() const { return shape; }

    /**
     * @brief Get the indices of neighboring tiles.
     * 
     * @return const std::vector<int>& Vector of neighboring tile indices.
     */
    const std::vector<int>& GetNeighbors() const { return neighbors; }

    /**
     * @brief Get the vertices that define the tile's boundary.
     * 
     * @return const std::vector<glm::vec3>& Vector of vertex positions (normalized to unit sphere).
     */
    const std::vector<glm::vec3>& GetVertices() const { return vertices; }

    /**
     * @brief Add a neighbor to this tile.
     * 
     * @param neighborIndex The index of the neighboring tile.
     */
    void AddNeighbor(int neighborIndex);

    /**
     * @brief Add a vertex to the tile's boundary.
     * 
     * @param vertex The position of the vertex (normalized to unit sphere).
     */
    void AddVertex(const glm::vec3& vertex);

    /**
     * @brief Set the vertices that define the tile's boundary.
     * 
     * @param vertices Vector of vertex positions (normalized to unit sphere).
     */
    void SetVertices(const std::vector<glm::vec3>& vertices);

    /**
     * @brief Set the neighbors of this tile.
     * 
     * @param neighbors Vector of neighboring tile indices.
     */
    void SetNeighbors(const std::vector<int>& neighbors);
    
    // Terrain data properties
    
    /**
     * @brief Get the elevation of this tile.
     * 
     * @return float The elevation value (0.0-1.0).
     */
    float GetElevation() const { return elevation; }
    
    /**
     * @brief Set the elevation of this tile.
     * 
     * @param elevation The elevation value (0.0-1.0).
     */
    void SetElevation(float elevation) { this->elevation = elevation; }
    
    /**
     * @brief Get the moisture level of this tile.
     * 
     * @return float The moisture value (0.0-1.0).
     */
    float GetMoisture() const { return moisture; }
    
    /**
     * @brief Set the moisture level of this tile.
     * 
     * @param moisture The moisture value (0.0-1.0).
     */
    void SetMoisture(float moisture) { this->moisture = moisture; }
    
    /**
     * @brief Get the temperature of this tile.
     * 
     * @return float The temperature value (0.0-1.0).
     */
    float GetTemperature() const { return temperature; }
    
    /**
     * @brief Set the temperature of this tile.
     * 
     * @param temperature The temperature value (0.0-1.0).
     */
    void SetTemperature(float temperature) { this->temperature = temperature; }

    /**
     * @brief Get the terrain type of this tile.
     * 
     * @return TerrainType The terrain type.
     */
    TerrainType GetTerrainType() const { return terrainType; }
    
    /**
     * @brief Set the terrain type of this tile.
     * 
     * @param terrainType The terrain type.
     */
    void SetTerrainType(TerrainType terrainType) { this->terrainType = terrainType; }
    
    /**
     * @brief Get the biome type of this tile.
     * 
     * @return BiomeType The biome type.
     */
    BiomeType GetBiomeType() const { return biomeType; }
    
    /**
     * @brief Set the biome type of this tile.
     * 
     * @param biomeType The biome type.
     */
    void SetBiomeType(BiomeType biomeType) { this->biomeType = biomeType; }

private:
    glm::vec3 center;                ///< Center position of the tile
    TileShape shape;                 ///< Shape of tile (Pentagon or Hexagon)
    std::vector<int> neighbors;      ///< Indices of neighboring tiles
    std::vector<glm::vec3> vertices; ///< Positions of the tile's boundary vertices
    
    // Terrain attributes
    float elevation = 0.5f;          ///< Elevation of the tile (0.0-1.0)
    float moisture = 0.5f;           ///< Moisture level of the tile (0.0-1.0)
    float temperature = 0.5f;        ///< Temperature of the tile (0.0-1.0)
    TerrainType terrainType = TerrainType::Lowland; ///< Type of terrain in this tile
    BiomeType biomeType = BiomeType::TemperateGrassland; ///< Biome type in this tile
};

} // namespace Generators
} // namespace WorldGen