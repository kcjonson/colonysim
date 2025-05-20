#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "../Generators/World.h"
#include "../../Game/World.h"
#include "TerrainTypes.h"

namespace WorldGen {
namespace Core {

/**
 * @brief Helper function to find the nearest tile in the generator world to a given point.
 * 
 * @param point 3D point on the unit sphere.
 * @param tiles Vector of generator tiles to search.
 * @return int Index of the nearest tile.
 */
int findNearestTile(const glm::vec3& point, const std::vector<WorldGen::Generators::Tile>& tiles);

/**
 * @brief Creates a Game::World object from a WorldGen::Generators::World object.
 * 
 * This function samples the surface of the generator world (which uses pentagon/hexagon tiles)
 * at the specified sample rate and produces game tiles (which are square). Depending on the
 * sample rate, a single world generator tile could produce many game tiles.
 * 
 * @param generatorWorld The source WorldGen::Generators::World object.
 * @param gameState Reference to the GameState object required for Game::World construction.
 * @param sampleRate The number of samples to take per unit distance on the sphere surface.
 *        Higher values result in more detailed game worlds.
 * @param camera Optional pointer to a Camera object for the Game::World.
 * @param window Optional pointer to a GLFW window for the Game::World.
 * @param seed Optional seed string for the Game::World.
 * @return std::unique_ptr<World> A new Game::World object.
 */
std::unique_ptr<World> createGameWorld(
    const WorldGen::Generators::World& generatorWorld,
    GameState& gameState,
    float sampleRate,
    Camera* camera = nullptr,
    GLFWwindow* window = nullptr,
    const std::string& seed = "DefaultSeed"
);

} // namespace Core
} // namespace WorldGen
