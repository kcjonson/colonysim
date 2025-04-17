#include "World.h"
#include "../../ConfigManager.h"
#include "../../Camera.h"
#include "../../VectorGraphics.h"
#include "Tile.h"
#include "../WorldGen/TerrainGenerator.h"
#include <iostream>
#include <algorithm>
#include <iomanip> // For std::setw and std::setprecision
#include <unordered_map>
#include <unordered_set>
#include <GLFW/glfw3.h>
#include <cmath> // For std::round

// Include shape classes for testing
#include "../../Rendering/Shapes/Rectangle.h"

// Include style classes
#include "../../Rendering/Styles/Shape.h"

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

glm::vec4 World::getCameraBounds() const {
    if (!camera) {
        // Return default bounds if camera is not set
        return glm::vec4(-10.0f, 10.0f, -10.0f, 10.0f);
    }

    // Use camera's projection properties directly
    float left = camera->getProjectionLeft();
    float right = camera->getProjectionRight();
    float bottom = camera->getProjectionBottom();
    float top = camera->getProjectionTop();

    // Get camera position
    glm::vec3 cameraPos = camera->getPosition();

    // Adjust bounds based on camera position (assuming orthographic)
    // The projection bounds are relative to the camera's center
    return glm::vec4(
        cameraPos.x + left,   // world left
        cameraPos.x + right,  // world right
        cameraPos.y + bottom, // world bottom
        cameraPos.y + top     // world top
    );
}

void World::render() {
    // Only update visibility if the camera view actually changed
    if (cameraViewChanged()) {
        updateTileVisibility();
    }
    worldLayer->render(false); // Render the layer, which respects item visibility
}

// Helper function to check if camera position or projection changed
bool World::cameraViewChanged() const {
    if (!camera) return false; // No camera, no change

    glm::vec3 currentPos = camera->getPosition();
    glm::vec4 currentBounds(
        camera->getProjectionLeft(),
        camera->getProjectionRight(),
        camera->getProjectionBottom(),
        camera->getProjectionTop()
    );

    // Use a small epsilon for floating-point comparisons
    const float epsilon = 0.001f; 

    bool positionChanged = glm::distance(currentPos, lastCameraPos) > epsilon;
    bool projectionChanged = 
        std::abs(currentBounds.x - lastCameraProjBounds.x) > epsilon ||
        std::abs(currentBounds.y - lastCameraProjBounds.y) > epsilon ||
        std::abs(currentBounds.z - lastCameraProjBounds.z) > epsilon ||
        std::abs(currentBounds.w - lastCameraProjBounds.w) > epsilon;

    return positionChanged || projectionChanged;
}

void World::updateTileVisibility() {
    if (!camera) return; // Need camera for bounds

    // --- This whole block now only runs if cameraViewChanged() was true --- 

    // Get current camera state
    glm::vec3 currentPos = camera->getPosition();
    glm::vec4 currentProjBounds(
        camera->getProjectionLeft(),
        camera->getProjectionRight(),
        camera->getProjectionBottom(),
        camera->getProjectionTop()
    );
    glm::vec4 currentWorldBounds = getCameraBounds(); // Calculate world bounds based on current state

    // Calculate visible tile range with overscan based on current world bounds
    int minX = static_cast<int>(std::floor(currentWorldBounds.x / TILE_SIZE)) - overscanAmount;
    int maxX = static_cast<int>(std::ceil(currentWorldBounds.y / TILE_SIZE)) + overscanAmount;
    int minY = static_cast<int>(std::floor(currentWorldBounds.z / TILE_SIZE)) - overscanAmount;
    int maxY = static_cast<int>(std::ceil(currentWorldBounds.w / TILE_SIZE)) + overscanAmount;

    // Determine the set of tiles that should be visible this frame
    currentVisibleTiles.clear();
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            auto pos = std::make_pair(x, y);
            if (terrainData.count(pos) > 0) {
                currentVisibleTiles.insert(pos);
            }
        }
    }

    // Iterate through tiles that *were* visible last frame
    for (const auto& pos : lastVisibleTiles) {
        if (currentVisibleTiles.find(pos) == currentVisibleTiles.end()) {
            auto tileIt = tiles.find(pos);
            if (tileIt != tiles.end() && tileIt->second->isVisible()) {
                tileIt->second->setVisible(false);
            }
        }
    }

    // Iterate through tiles that *should be* visible this frame
    for (const auto& pos : currentVisibleTiles) {
        auto tileIt = tiles.find(pos);
        if (tileIt == tiles.end()) {
            auto terrainIt = terrainData.find(pos);
            if (terrainIt != terrainData.end()) {
                const auto& data = terrainIt->second;
                glm::vec2 tilePosition(pos.first * TILE_SIZE, pos.second * TILE_SIZE);
                auto tile = std::make_shared<Rendering::Tile>(
                    tilePosition, data.height, data.resource, data.type, data.color
                );
                tiles[pos] = tile;
                worldLayer->addItem(tile);
                tile->setVisible(true);
            }
        } else {
            if (!tileIt->second->isVisible()) {
                tileIt->second->setVisible(true);
            }
        }
    }

    // Update lastVisibleTiles for the next frame
    lastVisibleTiles = currentVisibleTiles;

    // Cache the camera state for the next frame's check
    lastCameraPos = currentPos;
    lastCameraProjBounds = currentProjBounds;

    // --- End of block that runs only if camera view changed --- 
}

void World::logMemoryUsage() const {
    size_t totalTiles = tiles.size();
    size_t shownTiles = currentVisibleTiles.size();
    size_t totalShapes = 0;
    float tileMemoryKB = 0.0f; // Declare earlier
    float shapeMemoryKB = 0.0f; // Declare earlier
    float totalMemoryKB = 0.0f; // Declare earlier

    // --- Uncommented this block ---
    for (const auto& pos : currentVisibleTiles) {
        auto tileIt = tiles.find(pos);
        if (tileIt != tiles.end() && tileIt->second) { // Add null check for safety
            // Access children via the Layer base class interface
            totalShapes += tileIt->second->getChildren().size(); 
        }
    }
    // --- End of uncommented block ---

    // Calculate memory usage
    tileMemoryKB = std::round(static_cast<float>(totalTiles) * sizeof(Rendering::Tile) / 1024.0f);
    shapeMemoryKB = std::round(static_cast<float>(totalShapes) * sizeof(Rendering::Shapes::Shape) / 1024.0f); 
    totalMemoryKB = std::round(tileMemoryKB + shapeMemoryKB);
    
    gameState.set("world.totalTiles", std::to_string(totalTiles));
    gameState.set("world.shownTiles", std::to_string(shownTiles));
    gameState.set("world.totalShapes", std::to_string(totalShapes));
    gameState.set("world.tileMemKB", std::to_string(static_cast<int>(tileMemoryKB)) + " KB");
    gameState.set("world.shapeMemKB", std::to_string(static_cast<int>(shapeMemoryKB)) + " KB");
    gameState.set("world.totalMemKB", std::to_string(static_cast<int>(totalMemoryKB)) + " KB");
}

void World::setTerrainData(const std::unordered_map<std::pair<int, int>, WorldGen::TerrainData, std::hash<std::pair<int, int>>>& data) {
    terrainData = data;
    tiles.clear();
    worldLayer->clearItems(); // Clear items from the layer when terrain changes
    lastVisibleTiles.clear();
    currentVisibleTiles.clear();
    // Reset cached camera state when terrain changes
    lastCameraPos = glm::vec3(-1e9f); // Set to unlikely value to force update
    lastCameraProjBounds = glm::vec4(-1e9f);
}

void World::setCamera(Camera* cam) {
    camera = cam;  // Store the camera reference directly
    worldLayer->setCamera(cam);  // Also pass to the rendering layer
    // Reset cached camera state when camera changes
    if (camera) {
        lastCameraPos = camera->getPosition() + glm::vec3(1.0f); // Ensure it's different initially
        lastCameraProjBounds = glm::vec4(
            camera->getProjectionLeft() + 1.0f, // Ensure different
            camera->getProjectionRight(),
            camera->getProjectionBottom(),
            camera->getProjectionTop()
        );
        // Force initial update
        updateTileVisibility(); 
    } else {
        lastCameraPos = glm::vec3(0.0f);
        lastCameraProjBounds = glm::vec4(0.0f);
    }
}

void World::setWindow(GLFWwindow* win) {
    worldLayer->setWindow(win);
}