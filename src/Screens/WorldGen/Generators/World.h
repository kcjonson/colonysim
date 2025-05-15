#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <array>
#include "Tile.h"
#include "../ProgressTracker.h"

namespace WorldGen {

// Forward declaration
struct PlanetParameters;

namespace Generators {

/**
 * @brief Represents the world as a spherical grid of tiles.
 * 
 * This class generates and maintains a representation of the world as a
 * geodesic grid based on a subdivided icosahedron. The resulting grid
 * contains 12 pentagonal tiles and the rest are hexagons.
 */
class World {
public:
    /**
     * @brief Construct a new World with default parameters.
     * 
     * @param progressTracker Optional progress tracker to report generation progress.
     */
    World(std::shared_ptr<ProgressTracker> progressTracker = nullptr);

    /**
     * @brief Construct a new World with specific parameters.
     * 
     * @param params The parameters to use for world generation.
     * @param progressTracker Optional progress tracker to report generation progress.
     */
    World(const PlanetParameters& params, std::shared_ptr<ProgressTracker> progressTracker = nullptr);

    /**
     * @brief Generate the world geometry based on a subdivided icosahedron.
     * 
     * @param subdivisionLevel Number of times to subdivide the base icosahedron.
     * @param distortionFactor Factor controlling the amount of vertex position distortion (0-1).
     */
    void Generate(int subdivisionLevel, float distortionFactor);

    /**
     * @brief Generate the world geometry with progress tracking.
     * 
     * @param subdivisionLevel Number of times to subdivide the base icosahedron.
     * @param distortionFactor Factor controlling the amount of vertex position distortion (0-1).
     * @param progressTracker Optional progress tracker to report generation progress.
     */
    void Generate(int subdivisionLevel, float distortionFactor, std::shared_ptr<ProgressTracker> progressTracker);    // No need for a separate SetProgressTracker method, constructor handles it

    /**
     * @brief Get all tiles in the world.
     * 
     * @return const std::vector<Tile>& The world's tiles.
     */
    const std::vector<Tile>& GetTiles() const { return m_tiles; }

    /**
     * @brief Get the number of tiles in the world.
     * 
     * @return size_t The total number of tiles.
     */
    size_t GetTileCount() const { return m_tiles.size(); }

    /**
     * @brief Get the number of pentagon tiles (should be 12).
     * 
     * @return size_t The number of pentagon tiles.
     */
    size_t GetPentagonCount() const { return m_pentagonCount; }

    /**
     * @brief Get the number of hexagon tiles.
     * 
     * @return size_t The number of hexagon tiles.
     */
    size_t GetHexagonCount() const { return m_tiles.size() - m_pentagonCount; }

    /**
     * @brief Get the vertices from the initial icosahedron.
     * 
     * @return const std::vector<glm::vec3>& The original icosahedron vertices.
     */
    const std::vector<glm::vec3>& GetIcosahedronVertices() const { return m_icosahedronVertices; }

    /**
     * @brief Get the triangular faces from the initial icosahedron.
     * 
     * @return const std::vector<std::array<int, 3>>& The original icosahedron faces.
     */
    const std::vector<std::array<int, 3>>& GetIcosahedronFaces() const { return m_icosahedronFaces; }

    /**
     * @brief Get the world radius.
     * 
     * @return float The world radius.
     */
    float GetRadius() const { return m_radius; }

    /**
     * @brief Set the world radius.
     * 
     * @param radius The new world radius.
     */
    void SetRadius(float radius) { m_radius = radius; }

private:
    /**
     * @brief Create the base icosahedron.
     */
    void CreateIcosahedron();

    /**
     * @brief Subdivide the icosahedron to create a more detailed mesh.
     * 
     * @param level Number of subdivision iterations.
     * @param distortionFactor Factor controlling the amount of vertex position distortion (0-1).
     */
    void SubdivideIcosahedron(int level, float distortionFactor);

    /**
     * @brief Convert the triangular mesh into a dual polyhedron of pentagons and hexagons.
     */
    void TrianglesToTiles();

    /**
     * @brief Setup the neighborhood relationships between tiles.
     */
    void SetupTileNeighbors();
    
    /**
     * @brief Generate terrain data for all tiles.
     * 
     * This method calculates elevation, moisture, and temperature for all tiles.
     */
    void GenerateTerrainData();
    
    /**
     * @brief Smooth terrain data by averaging with neighbors.
     * 
     * This method smooths elevation, moisture, and temperature by averaging with neighboring tiles.
     */
    void SmoothTerrainData();

    /**
     * @brief Get the mid-point between two vertices with optional distortion.
     * 
     * @param v1 First vertex.
     * @param v2 Second vertex.
     * @param distortionFactor Factor controlling the amount of distortion (0-1).
     * @return glm::vec3 The mid-point, projected onto the unit sphere.
     */
    glm::vec3 GetMidPoint(const glm::vec3& v1, const glm::vec3& v2, float distortionFactor);

    /**
     * @brief Get or create a midpoint between two vertices.
     * 
     * @param v1 Index of first vertex.
     * @param v2 Index of second vertex.
     * @param distortionFactor Factor controlling the amount of distortion (0-1).
     * @return int Index of the midpoint vertex.
     */
    int GetMidPointIndex(int v1, int v2, float distortionFactor);

    /**
     * @brief Apply random distortion to a point.
     * 
     * @param point The original point.
     * @param magnitude The maximum magnitude of distortion.
     * @return glm::vec3 The distorted point.
     */
    glm::vec3 ApplyDistortion(const glm::vec3& point, float magnitude);

    std::vector<Tile> m_tiles;                     ///< All tiles in the world
    std::vector<glm::vec3> m_icosahedronVertices;  ///< Original icosahedron vertices
    std::vector<std::array<int, 3>> m_icosahedronFaces; ///< Original icosahedron faces as index triplets
    
    // Subdivision data structures
    std::vector<glm::vec3> m_subdivisionVertices;  ///< Vertices after subdivision
    std::vector<std::array<int, 3>> m_subdivisionFaces; ///< Faces after subdivision
      // Cache of midpoints to avoid duplicates during subdivision
    std::unordered_map<uint64_t, size_t> m_midPointCache;
    
    float m_radius;         ///< World radius
    size_t m_pentagonCount; ///< Count of pentagon tiles (should be 12)
    uint64_t m_seed;        ///< Seed for random distortion
    std::shared_ptr<ProgressTracker> m_progressTracker; ///< Progress tracking
};

} // namespace Generators
} // namespace WorldGen