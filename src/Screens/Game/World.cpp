#include "World.h"
#include "../../ConfigManager.h"
#include "../../Camera.h"
#include "../../VectorGraphics.h"
#include "Tile.h"
#include "../WorldGen/Generators/TerrainGenerator.h" // Updated path to TerrainGenerator.h
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

// Constructor takes GameState, seed, camera, and window
World::World(GameState& gameState, const std::string& seed, Camera* cam, GLFWwindow* win)
    : gameState(gameState),
      seed(seed),
      camera(cam), // Initialize camera first
      worldLayer(std::make_shared<Rendering::Layer>(50.0f, Rendering::ProjectionType::WorldSpace, cam, win)),
      // Initialize last camera state *after* camera is initialized
      lastCameraPos(cam ? cam->getPosition() + glm::vec3(1.0f) : glm::vec3(0.0f)), // Add null check
      lastCameraProjBounds(cam ? glm::vec4(cam->getProjectionLeft() + 1.0f, cam->getProjectionRight(), cam->getProjectionBottom(), cam->getProjectionTop()) : glm::vec4(0.0f)) // Add null check
{
    std::cout << "Initializing world with seed: " << seed << std::endl;
    // Constructor body can remain empty or contain other setup logic
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
    try {
        // Check if we have a valid world layer before rendering
        if (!worldLayer) {
            std::cerr << "ERROR: Attempting to render with null worldLayer" << std::endl;
            return;
        }
        
        // Check if we have any terrain data before trying to render
        if (terrainData.empty()) {
            std::cerr << "WARNING: No terrain data available in World::render" << std::endl;
        }
        
        // Only update visibility if the camera view actually changed
        if (cameraViewChanged()) {
            try {
                updateTileVisibility();
            } catch (const std::exception& e) {
                std::cerr << "ERROR: Exception during updateTileVisibility: " << e.what() << std::endl;
            }
        }
        
        // Render the layer, which respects item visibility
        worldLayer->render(false);
        
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception in World::render: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "ERROR: Unknown exception in World::render" << std::endl;
    }
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
    // Early return for no camera
    if (!camera) {
        std::cerr << "WARNING: No camera in updateTileVisibility" << std::endl;
        return;
    }

    // Early return for empty terrain data
    if (terrainData.empty()) {
        std::cerr << "WARNING: No terrain data in updateTileVisibility" << std::endl;
        return;
    }

    // --- This whole block now only runs if cameraViewChanged() was true --- 
    try {
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
        // Use double for intermediate calculations to avoid potential precision issues
        double minXDouble = std::floor(currentWorldBounds.x / TILE_SIZE) - overscanAmount;
        double maxXDouble = std::ceil(currentWorldBounds.y / TILE_SIZE) + overscanAmount;
        double minYDouble = std::floor(currentWorldBounds.z / TILE_SIZE) - overscanAmount;
        double maxYDouble = std::ceil(currentWorldBounds.w / TILE_SIZE) + overscanAmount;
        
        // Convert to integers with safety bounds
        int minX = static_cast<int>(std::max(minXDouble, -100000.0));
        int maxX = static_cast<int>(std::min(maxXDouble, 100000.0));
        int minY = static_cast<int>(std::max(minYDouble, -100000.0));
        int maxY = static_cast<int>(std::min(maxYDouble, 100000.0));    // Determine the set of tiles that should be visible this frame
        currentVisibleTiles.clear();
        
        // Limit the number of tiles to process to avoid excessive processing
        const int MAX_TILES_TO_CHECK = 10000; // Arbitrary limit to prevent too much iteration
        int tilesChecked = 0;
        
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                // Safety check to avoid excessive processing
                if (++tilesChecked > MAX_TILES_TO_CHECK) {
                    std::cout << "WARNING: Maximum tile check limit reached in updateTileVisibility" << std::endl;
                    break;
                }
                
                WorldGen::TileCoord coord{x, y}; // Use TileCoord
                if (terrainData.count(coord) > 0) {
                    currentVisibleTiles.insert(coord);
                }
            }
            
            // Break out of outer loop if we hit the limit
            if (tilesChecked > MAX_TILES_TO_CHECK) {
                break;
            }
        }
        
        std::cout << "Visibility update: checked " << tilesChecked << " tiles, found " 
                  << currentVisibleTiles.size() << " visible tiles" << std::endl;

        // Iterate through tiles that *were* visible last frame
        for (const auto& coord : lastVisibleTiles) { // Use TileCoord
            if (currentVisibleTiles.find(coord) == currentVisibleTiles.end()) {
                auto tileIt = tiles.find(coord);
                if (tileIt != tiles.end() && tileIt->second && tileIt->second->isVisible()) {
                    tileIt->second->setVisible(false);
                }
            }
        }        // Limit the number of new tiles to create per frame to avoid memory spikes
        const int MAX_NEW_TILES_PER_FRAME = 100;
        int newTilesCreated = 0;
        
        // Iterate through tiles that *should be* visible this frame
        for (const auto& coord : currentVisibleTiles) { // Use TileCoord
            auto tileIt = tiles.find(coord);
            if (tileIt == tiles.end()) {
                // If we've hit our limit of new tiles this frame, skip creating more
                if (newTilesCreated >= MAX_NEW_TILES_PER_FRAME) {
                    continue;
                }
                
                auto terrainIt = terrainData.find(coord);
                if (terrainIt != terrainData.end()) {
                    try {
                        const auto& data = terrainIt->second;
                        glm::vec2 tilePosition(coord.x * TILE_SIZE, coord.y * TILE_SIZE); // Use coord.x, coord.y
                        
                        auto tile = std::make_shared<Rendering::Tile>(
                            tilePosition, data.height, data.resource, data.type, data.color
                        );
                        
                        tiles[coord] = tile;
                        
                        // Only add the item if we have a valid worldLayer
                        if (worldLayer) {
                            worldLayer->addItem(tile);
                        } else {
                            std::cerr << "WARNING: Null worldLayer when adding tile" << std::endl;
                        }
                        
                        tile->setVisible(true);
                        newTilesCreated++;
                    } catch (const std::exception& e) {
                        std::cerr << "ERROR: Exception creating tile at " << coord.x << ", " << coord.y 
                                  << ": " << e.what() << std::endl;
                    }
                }
            } else if (tileIt->second) { // Check for null before using
                if (!tileIt->second->isVisible()) {
                    tileIt->second->setVisible(true);
                }
            }
        }
        
        if (newTilesCreated > 0) {
            std::cout << "Created " << newTilesCreated << " new tiles this frame" << std::endl;
        }

        // Update lastVisibleTiles for the next frame
        lastVisibleTiles = currentVisibleTiles;

        // Cache the camera state for the next frame's check
        lastCameraPos = currentPos;
        lastCameraProjBounds = currentProjBounds;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception in updateTileVisibility: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "ERROR: Unknown exception in updateTileVisibility" << std::endl;
    }

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
    for (const auto& coord : currentVisibleTiles) { // Use TileCoord
        auto tileIt = tiles.find(coord);
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

// Update signature to use TileCoord
void World::setTerrainData(const std::unordered_map<WorldGen::TileCoord, WorldGen::TerrainData>& data) {
    terrainData = data;
    tiles.clear();
    worldLayer->clearItems(); // Clear items from the layer when terrain changes
    lastVisibleTiles.clear();
    currentVisibleTiles.clear();
    // Res cached camera state when terrain changes
    lastCameraPos = glm::vec3(-1e9f); // Set to unlikely value to force update
    lastCameraProjBounds = glm::vec4(-1e9f);
}