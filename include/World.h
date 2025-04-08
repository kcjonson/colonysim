#pragma once

#include <vector>
#include <memory>
#include "EntityManager.h"
#include "VectorGraphics.h"

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

private:
    void generatePerlinNoise(std::vector<float>& noise, int width, int height, float scale);

    int width;
    int height;
    std::vector<float> terrain;
    std::vector<float> resources;
    EntityManager entityManager;
}; 