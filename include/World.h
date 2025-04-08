#pragma once

#include <unordered_map>
#include <utility>
#include <functional> // For std::hash
#include "EntityManager.h"
#include "VectorGraphics.h"
#include "Camera.h"

// Hash function for std::pair (since it's not provided by default)
namespace std {
    template <>
    struct hash<std::pair<int, int>> {
        size_t operator()(const std::pair<int, int>& p) const {
            return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
        }
    };
}

struct Tile {
    float height = 0.0f;
    float resource = 0.0f;
    int type = 0; // 0 = empty, 1 = terrain, etc.
    glm::vec4 color = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f); // Default green
    // Add other properties as needed
};

class World {
public:
    World();
    ~World();

    void update(float deltaTime);
    void render(VectorGraphics& graphics);
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

    // Tile management
    void setTile(int x, int y, const Tile& tile);
    Tile getTile(int x, int y) const;

private:
    void generatePerlinNoise(std::unordered_map<std::pair<int, int>, Tile>& noiseMap, int width, int height, float scale);

    int width = 100;
    int height = 100;
    std::unordered_map<std::pair<int, int>, Tile> tiles;
    EntityManager entityManager;
    int generateDistance = 200;
    void generateTilesInRadius();
    int overscanAmount = 2; // Number of extra tiles to render beyond visible area
    glm::vec4 getCameraBounds() const; // Helper to calculate visible area
    Camera camera;
}; 