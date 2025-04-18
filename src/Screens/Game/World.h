#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <functional> // For std::hash
#include "../../VectorGraphics.h"
#include "../../Camera.h"
#include "../../Rendering/Layer.h"
#include "Tile.h"
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "../../GameState.h"
#include "../WorldGen/TerrainGenerator.h" // Includes the hash specialization

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
    void setTerrainData(const std::unordered_map<std::pair<int, int>, WorldGen::TerrainData, std::hash<std::pair<int, int>>>& data);
    
    // Set camera and window for the world
    void setCamera(Camera* cam);
    void setWindow(GLFWwindow* win);

private:
    void updateTileVisibility(); // New method for visibility toggling
    bool cameraViewChanged() const; // Helper to check if camera moved/zoomed

    // Remove width and height
    std::unordered_map<std::pair<int, int>, WorldGen::TerrainData, std::hash<std::pair<int, int>>> terrainData;
    std::unordered_map<std::pair<int, int>, std::shared_ptr<Rendering::Tile>, std::hash<std::pair<int, int>>> tiles;
    int overscanAmount = 3; // Increased overscan
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
    std::unordered_set<std::pair<int, int>, std::hash<std::pair<int, int>>> lastVisibleTiles;
    std::unordered_set<std::pair<int, int>, std::hash<std::pair<int, int>>> currentVisibleTiles;
    GameState& gameState;
    float timeSinceLastLog = 0.0f;

    // For caching camera state to detect changes
    glm::vec3 lastCameraPos = glm::vec3(0.0f);
    glm::vec4 lastCameraProjBounds = glm::vec4(0.0f); // Store left, right, bottom, top
};