#include "World.h"
#include "ConfigManager.h"
#include "Camera.h"
#include "VectorGraphics.h"
#include "Tile.h"
#include "World/TerrainGenerator.h"
#include <iostream>
#include <algorithm>
#include <iomanip> // For std::setw and std::setprecision
#include <unordered_map>
#include <unordered_set>
#include <GLFW/glfw3.h>
#include <cmath> // For std::round

// Include shape classes for testing
#include "Rendering/Shapes/Rectangle.h"

// Include style classes
#include "Rendering/Styles/Shape.h"

World::World(GameState& gameState, const std::string& seed)
    : gameState(gameState)
    , seed(seed)
    , worldLayer(std::make_shared<Rendering::Layer>(50.0f, Rendering::ProjectionType::WorldSpace)) {
    std::cout << "Initializing world with seed: " << seed << std::endl;
}

bool World::initialize() {
    // We'll set the camera later in setCamera, so this is not needed here
    return true;
}

void World::update(float deltaTime) {
    // Update memory usage logging
    timeSinceLastLog += deltaTime;
    if (timeSinceLastLog >= 0.5f) { // Log every 0.5 seconds
        logMemoryUsage();
        timeSinceLastLog = 0.0f;
    }
}

// glm::vec4 World::getCameraBounds() const {
//     // Directly return the camera's current projection bounds
//     return glm::vec4(
//         camera->getProjectionLeft(),
//         camera->getProjectionRight(),
//         camera->getProjectionBottom(),
//         camera->getProjectionTop()
//     );
// }

glm::vec4 World::getCameraBounds() const {
    // Get the actual window size for direct pixel-to-world mapping
    int width, height;
    if (GLFWwindow* win = worldLayer->getWindow()) {
        glfwGetWindowSize(win, &width, &height);
    } else {
        // Fallback to ConfigManager if window not available
        width = ConfigManager::getInstance().getWindowWidth();
        height = ConfigManager::getInstance().getWindowHeight();
    }
    
    // Get half-dimensions for calculating bounds
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    
    // Get camera position - check that camera is valid
    glm::vec3 cameraPos(0.0f);
    if (camera) {
        cameraPos = camera->getPosition();
    }
    
    return glm::vec4(
        cameraPos.x - halfWidth,   // left
        cameraPos.x + halfWidth,   // right
        cameraPos.y - halfHeight,  // bottom
        cameraPos.y + halfHeight   // top
    );
}

void World::render() {
    // Get camera bounds
    glm::vec4 bounds = getCameraBounds();

    // Calculate visible tile range with increased overscan
    int minX = static_cast<int>(std::floor(bounds.x / TILE_SIZE)) - 3; // Increase overscan from 1 to 3
    int maxX = static_cast<int>(std::ceil(bounds.y / TILE_SIZE)) + 3;
    int minY = static_cast<int>(std::floor(bounds.z / TILE_SIZE)) - 3;
    int maxY = static_cast<int>(std::ceil(bounds.w / TILE_SIZE)) + 3;

    // Create a set of tile coordinates that should be visible this frame
    currentVisibleTiles.clear();

    // Update visible tiles and track which ones should be visible
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            auto pos = std::make_pair(x, y);
            auto terrainIt = terrainData.find(pos);
            if (terrainIt == terrainData.end()) continue;

            auto tileIt = tiles.find(pos);
            if (tileIt == tiles.end()) {
                // Create new tile if it doesn't exist
                const auto& data = terrainIt->second;
                glm::vec2 tilePosition(x * TILE_SIZE, y * TILE_SIZE);
                auto tile = std::make_shared<Rendering::Tile>(
                    tilePosition,  // position
                    data.height,   // height
                    data.resource, // resource
                    data.type,     // type
                    data.color     // color
                );
                tiles[pos] = tile; // Add the new tile to the tiles map
                currentVisibleTiles.insert(pos); // Add the new tile coordinates to the current visible tiles set
                worldLayer->addItem(tile); // Add the new tile to the world layer
            } else {
                // Mark tile as visible in our tracking set
                currentVisibleTiles.insert(pos);
                
                // Check if the tile was previously hidden
                if (!tileIt->second->isVisible()) {
                    // Set visible and add to worldLayer if it wasn't visible before
                    tileIt->second->setVisible(true);
                    worldLayer->addItem(tileIt->second);
                }
            }
            
        }
    }

    // Hide tiles that were visible last frame but aren't visible this frame
    for (const auto& pos : lastVisibleTiles) {
        if (currentVisibleTiles.find(pos) == currentVisibleTiles.end()) {
            auto tileIt = tiles.find(pos);
            if (tileIt != tiles.end()) {
                tileIt->second->setVisible(false);
                worldLayer->removeItem(tileIt->second);
            }
        }
    }

    // Update lastVisibleTiles for next frame
    // NOTE: unlike js this is a copy not just a pointer
    lastVisibleTiles = currentVisibleTiles;
    worldLayer->render(false);
}

void World::renderTiles() {
    // This is now handled in the render() method
}

void World::logMemoryUsage() const {
    size_t totalTiles = tiles.size();
    size_t shownTiles = currentVisibleTiles.size();
    size_t totalShapes = 0;
        
    for (const auto& pos : currentVisibleTiles) {
        auto tileIt = tiles.find(pos);
        if (tileIt != tiles.end()) {
            totalShapes += tileIt->second->getChildren().size();
        }
    }
    
    float tileMemoryKB = std::round(totalTiles * sizeof(Rendering::Tile) / 1024.0f);
    float shapeMemoryKB = std::round(totalShapes * sizeof(Rendering::Shapes::Shape) / 1024.0f);
    float totalMemoryKB = std::round(tileMemoryKB + shapeMemoryKB);
    
    gameState.set("world.totalTiles", std::to_string(totalTiles));
    gameState.set("world.shownTiles", std::to_string(shownTiles));
    gameState.set("world.totalShapes", std::to_string(totalShapes));
    gameState.set("world.tileMemKB", std::to_string(static_cast<int>(tileMemoryKB)) + " KB");
    gameState.set("world.shapeMemKB", std::to_string(static_cast<int>(shapeMemoryKB)) + " KB");
    gameState.set("world.totalMemKB", std::to_string(static_cast<int>(totalMemoryKB)) + " KB");
}

void World::setTerrainData(const std::unordered_map<std::pair<int, int>, WorldGen::TerrainData>& data) {
    terrainData = data;
    tiles.clear();
}

void World::setCamera(Camera* cam) {
    camera = cam;  // Store the camera reference directly
    worldLayer->setCamera(cam);  // Also pass to the rendering layer
}

void World::setWindow(GLFWwindow* win) {
    worldLayer->setWindow(win);
}