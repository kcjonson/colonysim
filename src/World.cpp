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
    // Is this needed? Isn't the camera already set in Game.cpp?
    worldLayer->setCamera(&camera);
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
    float viewWidth = ConfigManager::getInstance().getViewHeight() * camera.getAspectRatio();
    float viewHeight = ConfigManager::getInstance().getViewHeight();
    
    // Get camera position (convert to 2D if using orthographic projection)
    glm::vec3 cameraPos = camera.getPosition();
    
    return glm::vec4(
        cameraPos.x - viewWidth/2.0f,   // left
        cameraPos.x + viewWidth/2.0f,   // right
        cameraPos.y - viewHeight/2.0f,  // bottom
        cameraPos.y + viewHeight/2.0f   // top
    );
}

void World::render(VectorGraphics& graphics) {
    // Get camera bounds
    glm::vec4 bounds = getCameraBounds();

    // Draw camera bounds rectangle - convert from bounds to top-left and size
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
                .color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),     // Light blue with minimal transparency
                .borderColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // Red border with full opacity
                .borderWidth = 2.0f,                             // 2px border width
                .borderPosition = BorderPosition::Outside,       // Border outside the rectangle
                .cornerRadius = 5.0f                             // Rounded corners
            }),
            500.0f // Z-index
        );
        worldLayer->addItem(boundsRect);
    } else {
        // Just update position and size
        boundsRect->setPosition(topLeft);
        boundsRect->setSize(rectSize);
    }

    // Calculate visible tile range with overscan
    int minX = static_cast<int>(std::floor(bounds.x / TILE_SIZE)) - 1;
    int maxX = static_cast<int>(std::ceil(bounds.y / TILE_SIZE)) + 1;
    int minY = static_cast<int>(std::floor(bounds.z / TILE_SIZE)) - 1;
    int maxY = static_cast<int>(std::ceil(bounds.w / TILE_SIZE)) + 1;

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
    worldLayer->setCamera(cam);
}

void World::setWindow(GLFWwindow* win) {
    worldLayer->setWindow(win);
}

void World::setRenderer(Renderer* renderer) {
    worldLayer->setRenderer(renderer);
} 