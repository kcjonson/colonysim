#include "World.h"
#include "ConfigManager.h"
#include "Camera.h"
#include "VectorGraphics.h"
#include "Tile.h"
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>
#include <iomanip> // For std::setw and std::setprecision
#include <unordered_map>
#include <unordered_set>

// Include shape classes for testing
#include "Rendering/Shapes/Rectangle.h"

// Include style classes
#include "Rendering/Styles/Shape.h"

constexpr float PI = 3.14159265358979323846f;

// Improved noise2D with gradient noise
float noise2D(float x, float y, unsigned int seed) {
    // Grid cell coordinates
    int x0 = static_cast<int>(std::floor(x));
    int x1 = x0 + 1;
    int y0 = static_cast<int>(std::floor(y));
    int y1 = y0 + 1;

    // Fractional parts
    float fx = x - x0;
    float fy = y - y0;

    // Smoothstep interpolation
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);

    // Random gradients (pseudo-random but deterministic)
    auto random = [](int xi, int yi, unsigned int seed) {
        float val = std::sin(xi * 12.9898f + yi * 78.233f + seed) * 43758.5453f;
        return val - std::floor(val);
    };

    // Dot products with gradients
    float n00 = random(x0, y0, seed) * 2.0f - 1.0f;
    float n01 = random(x0, y1, seed) * 2.0f - 1.0f;
    float n10 = random(x1, y0, seed) * 2.0f - 1.0f;
    float n11 = random(x1, y1, seed) * 2.0f - 1.0f;

    // Bilinear interpolation
    float nx0 = n00 + fx * (n10 - n00);
    float nx1 = n01 + fx * (n11 - n01);
    return nx0 + fy * (nx1 - nx0);
}

// Fractional Brownian Motion for terrain generation
float fbm(float x, float y, int octaves, float persistence, unsigned int seed) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += noise2D(x * frequency, y * frequency, seed) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
 
    return total / maxValue;
}

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
    std::cout << "Generating terrain..." << std::endl;
    generateTilesInRadius();
}

struct TerrainData {
    float height;
    float resource;
    int type;
    glm::vec4 color;
};

void World::generateTilesInRadius() {

/* 
    TODO: Generate a spherical world
    - divide the sphere into a grid of hexagons
    - generate major world features in the hexagons
    - each hexagon will have a number of world features (e.g. it can have a mountain, river and forest)


    TODO: generate sectors of terrain
    - each sector should be a hexagon
    - sectors may have a number of influence regions around their edge
    - the influence regions will be expressed as a side (top, bottom, left, right) and a start/end distance from the edge vector start
    - the influence points will be used to connect features between sectors (like rivers)

    TODO: generate the tiles within each sector
    - the tiles will be generated respecting the edge influence regions


*/

    unsigned int hashedSeed = getHashedSeed(); // Get numeric hash
    terrainData.clear();

    for (int y = -generateDistance; y <= generateDistance; y++) {
        for (int x = -generateDistance; x <= generateDistance; x++) {
            float nx = x * 0.05f;
            float ny = y * 0.05f;

            // Pass hashedSeed to fbm
            float heightValue = fbm(nx, ny, 4, 0.5f, hashedSeed);
            heightValue = (heightValue + 1.0f) * 0.5f;
            float resourceValue = fbm(nx * 0.5f, ny * 0.5f, 4, 0.5f, hashedSeed);

            TerrainData data;
            data.height = heightValue;
            data.resource = resourceValue;

            // Smooth terrain transitions
            if (heightValue > 0.7f) {
                data.type = 2; // Mountain
                data.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
            } 
            else if (heightValue > 0.5f) {
                data.type = 1; // Land
                float blend = (heightValue - 0.5f) / 0.2f;
                data.color = glm::mix(
                    glm::vec4(0.0f, 0.5f, 0.0f, 1.0f),
                    glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
                    blend
                );
            }
            else {
                data.type = 0; // Water
                float blend = heightValue / 0.5f;
                data.color = glm::mix(
                    glm::vec4(0.0f, 0.2f, 0.5f, 1.0f),
                    glm::vec4(0.0f, 0.5f, 0.8f, 1.0f),
                    blend
                );
            }

            // Resource tint
            if (resourceValue > 0.5f) {
                data.color.r = resourceValue;
            }

            terrainData[{x, y}] = data;
        }
    }
}

// Hash the string seed to a numeric value
unsigned int World::getHashedSeed() const {
    if (seed.empty()) return 0; // Default seed if empty

    // Simple hash function (FNV-1a)
    unsigned int hash = 2166136261u;
    for (char c : seed) {
        hash ^= static_cast<unsigned int>(c);
        hash *= 16777619u;
    }
    return hash;
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