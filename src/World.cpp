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

void World::render(VectorGraphics& graphics) {
    // Get camera bounds
    glm::vec4 bounds = getCameraBounds();

    // Debug camera bounds movement (only log significant changes)
    if (camera) {
        static glm::vec3 lastPos = camera->getPosition();
        glm::vec3 currentPos = camera->getPosition();
        
        // Log only if moved more than 50 units to reduce spam
        if (glm::distance(lastPos, currentPos) > 50.0f) {
            std::cout << "Camera at: (" << int(currentPos.x) << ", " << int(currentPos.y) << ") - viewing area updated" << std::endl;
            lastPos = currentPos;
        }
    }

    // Debug: Draw camera bounds rectangle - convert from bounds to top-left and size
    // The inset is just to make the rectangle visible
    glm::vec2 topLeft(bounds.x + 5.0f, bounds.z + 5.0f); // Add 5px inset
    glm::vec2 rectSize(bounds.y - bounds.x - 10.0f, bounds.w - bounds.z - 10.0f);
    
    // Update the existing rectangle or create a new one if it doesn't exist
    static std::shared_ptr<Rendering::Shapes::Rectangle> boundsRect = nullptr;
    if (!boundsRect) {
        std::cout << "Creating camera bounds debug rectangle" << std::endl;
        boundsRect = std::make_shared<Rendering::Shapes::Rectangle>(
            topLeft,
            rectSize,
            Rendering::Styles::Rectangle({
                .color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),     // Transparent fill
                .borderColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // Red border
                .borderWidth = 2.0f,                             // 2px border width
                .borderPosition = BorderPosition::Inside,        // Border inside the rectangle
                .cornerRadius = 0.0f                             // No rounded corners
            }),
            500.0f // Z-index
        );
        worldLayer->addItem(boundsRect);
    } else {
        // Just update position and size
        boundsRect->setPosition(topLeft);
        boundsRect->setSize(rectSize);
    }

    // Calculate visible tile range with increased overscan
    int minX = static_cast<int>(std::floor(bounds.x / TILE_SIZE)) - 3; // Increase overscan from 1 to 3
    int maxX = static_cast<int>(std::ceil(bounds.y / TILE_SIZE)) + 3;
    int minY = static_cast<int>(std::floor(bounds.z / TILE_SIZE)) - 3;
    int maxY = static_cast<int>(std::ceil(bounds.w / TILE_SIZE)) + 3;

    // Create a set of tile coordinates that should be visible this frame
    std::unordered_set<std::pair<int, int>> currentVisibleTiles;

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
    lastVisibleTiles = std::move(currentVisibleTiles);

    // Add world objects to the batch
    worldLayer->render(graphics);

    // Get the view and projection matrices
    // TODO: this should be done in the layer
    glm::mat4 viewMatrix = worldLayer->getViewMatrix();
    glm::mat4 projectionMatrix = worldLayer->getProjectionMatrix();

    // Finalize the world batch with the world-space projection
    graphics.render(viewMatrix, projectionMatrix);
}

void World::logMemoryUsage() const {
    size_t tileCount = tiles.size();
    size_t totalShapes = 0;
    
    for (const auto& [pos, tile] : tiles) {
        totalShapes += tile->getChildren().size();
    }
    
    float tileMemoryKB = tileCount * sizeof(Rendering::Tile) / 1024.0f;
    float shapeMemoryKB = totalShapes * sizeof(Rendering::Shapes::Shape) / 1024.0f;
    float totalMemoryKB = tileMemoryKB + shapeMemoryKB;
    
    gameState.set("world.tileCount", std::to_string(tileCount));
    gameState.set("world.totalShapes", std::to_string(totalShapes));
    gameState.set("world.tileMemoryKB", std::to_string(tileMemoryKB) + " KB");
    gameState.set("world.shapeMemoryKB", std::to_string(shapeMemoryKB) + " KB");
    gameState.set("world.totalMemoryKB", std::to_string(totalMemoryKB) + " KB");
}

void World::generateTerrain() {
    unsigned int hashedSeed = WorldGen::TerrainGenerator::getHashedSeed(seed);
    WorldGen::TerrainGenerator::generateTerrain(terrainData, generateDistance, hashedSeed);
}

void World::generateTilesInRadius() {
    // This function is now handled by TerrainGenerator::generateTerrain
    // We just need to call generateTerrain() which calls it
    generateTerrain();
}

void World::setCamera(Camera* cam) {
    camera = cam;  // Store the camera reference directly
    worldLayer->setCamera(cam);  // Also pass to the rendering layer
}

void World::setWindow(GLFWwindow* win) {
    worldLayer->setWindow(win);
}

void World::setRenderer(Renderer* renderer) {
    worldLayer->setRenderer(renderer);
} 