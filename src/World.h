#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <functional> // For std::hash
#include "VectorGraphics.h"
#include "Camera.h"
#include "Rendering/Layer.h"
#include "Screens/Game/Tile.h"
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "GameState.h"
#include "Screens/WorldGen/TerrainGenerator.h"

class World {
public:
    World(GameState& gameState, const std::string& seed = "I am a seed, how novel!");
    ~World() = default;

    // Initialize world resources
    bool initialize();
    
    void update(float deltaTime);
    
    // Main render method - uses VectorGraphics singleton
    void render();
    
    // Terrain
    float getTerrainHeight(int x, int y) const;
    float getResourceAmount(int x, int y) const;
    void setTerrainData(const std::unordered_map<std::pair<int, int>, WorldGen::TerrainData>& data);
    
    // Set camera and window for the world
    void setCamera(Camera* cam);
    void setWindow(GLFWwindow* win);

private:
    void renderTiles();
    // Remove width and height
    std::unordered_map<std::pair<int, int>, WorldGen::TerrainData> terrainData;
    std::unordered_map<std::pair<int, int>, std::shared_ptr<Rendering::Tile>> tiles;
    int overscanAmount = 2; // Number of extra tiles to render beyond visible area
    glm::vec4 getCameraBounds() const; // Helper to calculate visible area
    Camera* camera = nullptr; // Use a pointer instead of a direct instance
    std::string seed;
    
    // For organizing world rendering
    std::shared_ptr<Rendering::Layer> worldLayer;
    
    // Helper to log memory usage
    void logMemoryUsage() const;
    
    // Constants
    static constexpr float TILE_SIZE = 20.0f;
    static constexpr float TILE_Z_INDEX = 0.1f;
    std::unordered_set<std::pair<int, int>> lastVisibleTiles;
    std::unordered_set<std::pair<int, int>> currentVisibleTiles;
    GameState& gameState;
    float timeSinceLastLog = 0.0f;
};