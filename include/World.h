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
    
    float getTerrainHeight(int x, int y) const;
    float getResourceAmount(int x, int y) const;
    void removeEntity(size_t index);
    void generateTerrain();
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    EntityManager& getEntityManager() { return entityManager; }
    const EntityManager& getEntityManager() const { return entityManager; }

private:
    void generatePerlinNoise(std::vector<float>& noise, int width, int height, float scale);

    int width;
    int height;
    std::vector<float> terrain;
    std::vector<float> resources;
    EntityManager entityManager;
}; 