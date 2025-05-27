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

// DEPRECATED: This function is deprecated. Use the new World constructor with generateInitialChunk instead.
// The new World class uses chunked loading for better performance and memory usage.
/*
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

} // namespace Core
} // namespace WorldGen
