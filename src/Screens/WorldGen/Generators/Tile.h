#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

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
    const glm::vec3& GetCenter() const { return m_center; }    /**
     * @brief Get the shape of tile.
     * 
     * @return TileShape The tile shape (Pentagon or Hexagon).
     */
    TileShape GetShape() const { return m_shape; }

    /**
     * @brief Get the indices of neighboring tiles.
     * 
     * @return const std::vector<int>& Vector of neighboring tile indices.
     */
    const std::vector<int>& GetNeighbors() const { return m_neighbors; }

    /**
     * @brief Get the vertices that define the tile's boundary.
     * 
     * @return const std::vector<glm::vec3>& Vector of vertex positions (normalized to unit sphere).
     */
    const std::vector<glm::vec3>& GetVertices() const { return m_vertices; }

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
    float GetElevation() const { return m_elevation; }
    
    /**
     * @brief Set the elevation of this tile.
     * 
     * @param elevation The elevation value (0.0-1.0).
     */
    void SetElevation(float elevation) { m_elevation = elevation; }
    
    /**
     * @brief Get the moisture level of this tile.
     * 
     * @return float The moisture value (0.0-1.0).
     */
    float GetMoisture() const { return m_moisture; }
    
    /**
     * @brief Set the moisture level of this tile.
     * 
     * @param moisture The moisture value (0.0-1.0).
     */
    void SetMoisture(float moisture) { m_moisture = moisture; }
    
    /**
     * @brief Get the temperature of this tile.
     * 
     * @return float The temperature value (0.0-1.0).
     */
    float GetTemperature() const { return m_temperature; }
    
    /**
     * @brief Set the temperature of this tile.
     * 
     * @param temperature The temperature value (0.0-1.0).
     */
    void SetTemperature(float temperature) { m_temperature = temperature; }

private:
    glm::vec3 m_center;                ///< Center position of the tile
    TileShape m_shape;                 ///< Shape of tile (Pentagon or Hexagon)
    std::vector<int> m_neighbors;      ///< Indices of neighboring tiles
    std::vector<glm::vec3> m_vertices; ///< Positions of the tile's boundary vertices
    
    // Terrain attributes
    float m_elevation = 0.5f;          ///< Elevation of the tile (0.0-1.0)
    float m_moisture = 0.5f;           ///< Moisture level of the tile (0.0-1.0)
    float m_temperature = 0.5f;        ///< Temperature of the tile (0.0-1.0)
};

} // namespace Generators
} // namespace WorldGen