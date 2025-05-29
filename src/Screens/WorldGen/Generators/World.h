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
     * @brief Construct a new World with specific parameters.
     * 
     * @param params The parameters to use for world generation.
     * @param seed Random seed for world generation.
     * @param progressTracker Progress tracker to report generation progress.
     */
    World(const PlanetParameters& params, uint64_t seed, std::shared_ptr<ProgressTracker> progressTracker);    /**
     * @brief Generate the world geometry with progress tracking.
     * 
     * @param subdivisionLevel Number of times to subdivide the base icosahedron.
     * @param distortionFactor Factor controlling the amount of vertex position distortion (0-1).
     * @param progressTracker Progress tracker to report generation progress.
     */
    void Generate(int subdivisionLevel, float distortionFactor, std::shared_ptr<ProgressTracker> progressTracker);

    /**
     * @brief Get all tiles in the world.
     * 
     * @return const std::vector<Tile>& The world's tiles.
     */
    const std::vector<Tile>& GetTiles() const { return tiles; }

    /**
     * @brief Get the number of tiles in the world.
     * 
     * @return size_t The total number of tiles.
     */
    size_t GetTileCount() const { return tiles.size(); }

    /**
     * @brief Get the number of pentagon tiles (should be 12).
     * 
     * @return size_t The number of pentagon tiles.
     */
    size_t GetPentagonCount() const { return pentagonCount; }

    /**
     * @brief Get the number of hexagon tiles.
     * 
     * @return size_t The number of hexagon tiles.
     */
    size_t GetHexagonCount() const { return tiles.size() - pentagonCount; }

    /**
     * @brief Get the vertices from the initial icosahedron.
     * 
     * @return const std::vector<glm::vec3>& The original icosahedron vertices.
     */
    const std::vector<glm::vec3>& GetIcosahedronVertices() const { return icosahedronVertices; }

    /**
     * @brief Get the triangular faces from the initial icosahedron.
     * 
     * @return const std::vector<std::array<int, 3>>& The original icosahedron faces.
     */
    const std::vector<std::array<int, 3>>& GetIcosahedronFaces() const { return icosahedronFaces; }

    /**
     * @brief Get the world radius.
     * 
     * @return float The world radius.
     */
    float GetRadius() const { return radius; }

    /**
     * @brief Set the world radius.
     * 
     * @param radius The new world radius.
     */
    void SetRadius(float radius) { this->radius = radius; }
    
    /**
     * @brief Find which tile contains a given point on the sphere.
     * 
     * This method can start from a previously known tile and search locally through 
     * its neighbors. This is much more efficient than a global search when sampling 
     * sequential points that are near each other (like when generating a chunk).
     * 
     * Example usage:
     *   int currentTile = -1;  // Start with no knowledge
     *   for each sample point in chunk:
     *     currentTile = FindTileContainingPoint(point, currentTile);
     *     // currentTile now holds the tile containing this point
     *     // and will be used as the starting point for the next search
     * 
     * @param point Point on the unit sphere to test
     * @param previousTileIndex Index of a nearby tile to start search from (-1 for global search)
     * @return Index of the tile containing the point, or -1 if not found
     */
    int FindTileContainingPoint(const glm::vec3& point, int previousTileIndex = -1) const;

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
    
    /**
     * @brief Check if a point on the sphere belongs to a specific tile.
     * 
     * Uses a Voronoi cell test: the point belongs to this tile if the tile's
     * center is closer than any neighboring tile's center.
     * 
     * @param point Point on the unit sphere
     * @param tileIndex Index of the tile to test
     * @return true if the point is inside this tile's region
     */
    bool isPointInTile(const glm::vec3& point, int tileIndex) const;

    std::vector<Tile> tiles;                     ///< All tiles in the world
    std::vector<glm::vec3> icosahedronVertices;  ///< Original icosahedron vertices
    std::vector<std::array<int, 3>> icosahedronFaces; ///< Original icosahedron faces as index triplets
    
    // Subdivision data structures
    std::vector<glm::vec3> subdivisionVertices;  ///< Vertices after subdivision
    std::vector<std::array<int, 3>> subdivisionFaces; ///< Faces after subdivision
      // Cache of midpoints to avoid duplicates during subdivision
    std::unordered_map<uint64_t, size_t> midPointCache;
    
    float radius;         ///< World radius
    size_t pentagonCount; ///< Count of pentagon tiles (should be 12)
    uint64_t seed;        ///< Seed for random distortion
    std::shared_ptr<ProgressTracker> progressTracker; ///< Progress tracking
};

} // namespace Generators
} // namespace WorldGen