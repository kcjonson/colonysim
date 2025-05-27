// DEPRECATED: This file is deprecated and only kept for reference.
// Use World.h (formerly ChunkedWorld) instead.
// This old implementation loads the entire world at once which doesn't scale well.

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
#include "../WorldGen/Core/TerrainTypes.h" // For TileCoord and TerrainData
#include "../WorldGen/Generators/TerrainGenerator.h" // Updated path to TerrainGenerator.h

// DEPRECATED: This class is deprecated. Use World (formerly ChunkedWorld) instead.
class WorldOld {
public:
    // Update constructor declaration
    WorldOld(GameState& gameState, const std::string& seed = "I am a seed, how novel!", Camera* cam = nullptr, GLFWwindow* win = nullptr);
    ~WorldOld() = default;

    // Initialize world resources
    bool initialize();
    
    void update(float deltaTime);
    
    // Main render method - uses VectorGraphics singleton
    void render();
    
    // Terrain
    float getTerrainHeight(int x, int y) const;
    float getResourceAmount(int x, int y) const;
    void setTerrainData(const std::unordered_map<WorldGen::TileCoord, WorldGen::TerrainData>& data); // Use TileCoord
    
    // Remove setCamera and setWindow declarations if no longer needed

private:
    void updateTileVisibility(); // New method for visibility toggling
    bool cameraViewChanged() const; // Helper to check if camera moved/zoomed

    // Remove width and height
    std::unordered_map<WorldGen::TileCoord, WorldGen::TerrainData> terrainData; // Use TileCoord
    std::unordered_map<WorldGen::TileCoord, std::shared_ptr<Rendering::Tile>> tiles; // Use TileCoord
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
    std::unordered_set<WorldGen::TileCoord> lastVisibleTiles; // Use TileCoord
    std::unordered_set<WorldGen::TileCoord> currentVisibleTiles; // Use TileCoord
    GameState& gameState;
    float timeSinceLastLog = 0.0f;

    // For caching camera state to detect changes
    glm::vec3 lastCameraPos = glm::vec3(0.0f);
    glm::vec4 lastCameraProjBounds = glm::vec4(0.0f); // Store left, right, bottom, top
};