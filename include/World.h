#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <functional> // For std::hash
#include "VectorGraphics.h"
#include "Camera.h"
#include "Rendering/Layer.h"
#include "Tile.h"
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "GameState.h"
#include "World/TerrainGenerator.h"

class World {
public:
    World(GameState& gameState, const std::string& seed = "I am a seed, how novel!");
    ~World() = default;

    // Initialize world resources
    bool initialize();
    
    void update(float deltaTime);
    
    // Main render method - no longer requires view/projection matrices
    void render(VectorGraphics& graphics);
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Terrain
    float getTerrainHeight(int x, int y) const;
    float getResourceAmount(int x, int y) const;
    void generateTerrain();
    
    // Set camera, window, and renderer for the world
    void setCamera(Camera* cam);
    void setWindow(GLFWwindow* win);
    void setRenderer(Renderer* renderer);

private:
    void generatePerlinNoise(std::unordered_map<std::pair<int, int>, WorldGen::TerrainData>& noiseMap, int width, int height, float scale);
    void renderTiles(VectorGraphics& graphics);

    int width = 100;
    int height = 100;
    std::unordered_map<std::pair<int, int>, WorldGen::TerrainData> terrainData;
    std::unordered_map<std::pair<int, int>, std::shared_ptr<Rendering::Tile>> tiles;
    int generateDistance = 200;
    void generateTilesInRadius();
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
    GameState& gameState;
    float timeSinceLastLog = 0.0f;
}; 