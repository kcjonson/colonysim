#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "../Generators/World.h"
#include "TerrainTypes.h"
#include "ChunkTypes.h"

// Forward declarations
class World;
class GameState;
class Camera;
struct GLFWwindow;

namespace WorldGen {
namespace Core {

/**
 * @brief Helper function to find the nearest tile in the generator world to a given point.
 * 
 * @param point 3D point on the unit sphere.
 * @param tiles Vector of generator tiles to search.
 * @return int Index of the nearest tile.
 */
int findNearestTile(const glm::vec3& point, const std::vector<::WorldGen::Generators::Tile>& tiles);

/*
// ============================================================================
// DEPRECATED FUNCTION - DO NOT USE
// ============================================================================
// This function is deprecated and unused. Use the new chunked generation system instead.
std::unique_ptr<World> createGameWorld(
    const ::WorldGen::Generators::World& worldGenerator,
    GameState& gameState,
    float sampleRate,
    Camera* camera = nullptr,
    GLFWwindow* window = nullptr,
    const std::string& seed = "DefaultSeed"
);
*/

/**
 * @brief Generate the initial chunk at the landing location.
 * 
 * This is a convenience function that generates the first chunk centered
 * at the player's landing location on the sphere.
 * 
 * @param worldGenerator The source WorldGen::Generators::World object containing spherical world data.
 * @param landingLocation The 3D point on the unit sphere where the player lands.
 * @return std::unique_ptr<ChunkData> The generated chunk data.
 */
std::unique_ptr<ChunkData> generateInitialChunk(
    const ::WorldGen::Generators::World& worldGenerator,
    const glm::vec3& landingLocation
);

/**
 * @brief Convert 3D sphere position to longitude/latitude in radians.
 * 
 * COORDINATE SYSTEM FOUNDATION:
 * This is the core conversion function that all other coordinate transformations should use.
 * It extracts longitude and latitude from a 3D point on the unit sphere.
 * 
 * ⚠️  IMPORTANT: See docs/ChunkedWorldImplementation.md "Coordinate Systems" section
 *     for complete documentation of the coordinate system design and rationale.
 * 
 * SPHERE COORDINATE CONVENTION:
 * - Sphere position (1,0,0) = Prime meridian (0°) and Equator (0°)
 * - Sphere position (0,1,0) = North pole (90° latitude)
 * - Sphere position (0,0,1) = 90° east longitude on equator
 * - Sphere position (-1,0,0) = 180° longitude (international date line)
 * 
 * OUTPUT RANGES:
 * - Longitude (X): [-π, +π] radians (east is positive)
 * - Latitude (Y):  [-π/2, +π/2] radians (north is positive)
 * 
 * @param spherePos Normalized position on unit sphere
 * @return glm::vec2(longitude, latitude) in radians
 */
glm::vec2 sphereToLatLong(const glm::vec3& spherePos);

/**
 * @brief Convert sphere position to world coordinates in meters.
 * 
 * ⚠️  IMPORTANT: See docs/ChunkedWorldImplementation.md "Coordinate Systems" section
 *     for complete documentation. This is part of the 4-tier coordinate system.
 * 
 * WORLD COORDINATE SYSTEM:
 * - Origin (0,0) is at prime meridian and equator: sphere position (1,0,0)
 * - X-axis represents longitude in meters (east/west movement)
 * - Y-axis represents latitude in meters (north/south movement)
 * - Uses planet's physical radius to convert angular coordinates to linear distance
 * - Accepts equirectangular projection distortion as reasonable trade-off
 * 
 * RELATIONSHIP TO OTHER SYSTEMS:
 * - World coordinates are the intermediate system for spatial calculations
 * - Chunks are positioned in world coordinates
 * - Game coordinates (pixels) are derived from world coordinates
 * - Avoids precision issues with planet-scale pixel coordinates
 * 
 * @param spherePos Normalized position on unit sphere
 * @return World position in meters from origin (prime meridian/equator)
 */
glm::vec2 sphereToWorld(const glm::vec3& spherePos);

/**
 * @brief Convert world coordinates to game coordinates in pixels.
 * 
 * ⚠️  IMPORTANT: See docs/ChunkedWorldImplementation.md "Coordinate Systems" section
 *     for complete documentation. This is the final step in the coordinate chain.
 * 
 * GAME COORDINATE SYSTEM:
 * - Final coordinate system used for rendering tiles and positioning camera
 * - Origin typically centered on current play area to maintain precision
 * - Units are pixels, with conversion based on tile size and density
 * - Prevents trillion-pixel coordinates that cause floating-point errors
 * 
 * CONVERSION FORMULA:
 * - 1 meter = tilesPerMeter * tileSize pixels
 * - Default: 1 meter = 1.0 tiles/meter * 10 pixels/tile = 10 pixels
 * 
 * SCALE MANAGEMENT:
 * - Keeps coordinates local and manageable
 * - Essential for planet-scale worlds where absolute pixel coordinates
 *   would exceed floating-point precision
 * 
 * @param worldPos Position in world coordinates (meters)
 * @return Position in game coordinates (pixels)
 */
glm::vec2 worldToGame(const glm::vec2& worldPos);

} // namespace Core
} // namespace WorldGen
