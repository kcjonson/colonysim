#pragma once

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <functional> // For std::hash
#include "EntityManager.h"
#include "VectorGraphics.h"
#include "Camera.h"
#include "Rendering/Layer.h"
#include "Tile.h"
#include <string>
#include <memory>
#include <glm/glm.hpp>

// Hash function for std::pair (since it's not provided by default)
namespace std {
    template <>
    struct hash<std::pair<int, int>> {
        size_t operator()(const std::pair<int, int>& p) const {
            return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
        }
    };
}

class World {
public:
    World(const std::string& seed = "I am a seed, how novel!");
    ~World() = default;

    // Initialize world resources
    bool initialize();
    
    void update(float deltaTime);
    
    // Main render method - calls renderWorld or renderUI based on which graphics instance is passed
    void render(VectorGraphics& graphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Terrain
    float getTerrainHeight(int x, int y) const;
    float getResourceAmount(int x, int y) const;
    void generateTerrain();
    

    // Entities
    EntityManager& getEntityManager() { return entityManager; }
    const EntityManager& getEntityManager() const { return entityManager; }
    size_t createEntity(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    Entity* getEntity(size_t index);
    void removeEntity(size_t index);


private:
    struct TerrainData {
        float height;
        float resource;
        int type;
        glm::vec4 color;
    };

    void generatePerlinNoise(std::unordered_map<std::pair<int, int>, TerrainData>& noiseMap, int width, int height, float scale);
    void renderTiles(VectorGraphics& graphics);
    void renderEntities(VectorGraphics& graphics);

    int width = 100;
    int height = 100;
    std::unordered_map<std::pair<int, int>, TerrainData> terrainData;
    std::unordered_map<std::pair<int, int>, std::shared_ptr<Rendering::Tile>> tiles;
    EntityManager entityManager;
    int generateDistance = 200;
    void generateTilesInRadius();
    int overscanAmount = 2; // Number of extra tiles to render beyond visible area
    glm::vec4 getCameraBounds() const; // Helper to calculate visible area
    Camera camera;
    std::string seed;
    unsigned int getHashedSeed() const;
    
    // For organizing world rendering
    std::shared_ptr<Rendering::Layer> worldLayer;
    
    // Helper to log memory usage
    void logMemoryUsage() const;
    
    // Constants
    static constexpr float TILE_SIZE = 20.0f;
    static constexpr float TILE_Z_INDEX = 0.1f;
    std::unordered_set<std::pair<int, int>> lastVisibleTiles;
}; 