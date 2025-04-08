#include "World.h"
#include "VectorGraphics.h"
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>

constexpr float PI = 3.14159265358979323846f;

// Simple 2D noise function
float noise2D(float x, float y) {
    return std::sin(x * 0.01f) * std::cos(y * 0.01f);
}

// Fractional Brownian Motion for terrain generation
float fbm(float x, float y, int octaves, float persistence) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += noise2D(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }

    return total / maxValue;
}

World::World() : width(100), height(100) {
    terrain.resize(width * height);
    resources.resize(width * height);
    generateTerrain();
}

World::~World() = default;

void World::update(float deltaTime) {
    std::cout << "Updating world state..." << std::endl;
    entityManager.update(deltaTime);
}

void World::render(VectorGraphics& graphics) {
    std::cout << "Rendering world..." << std::endl;
    
    // Draw grid
    for (int x = -10; x <= 10; x++) {
        for (int y = -10; y <= 10; y++) {
            glm::vec2 pos(x * 1.0f, y * 1.0f);
            glm::vec2 size(0.1f, 0.1f);
            glm::vec4 color(0.5f, 0.5f, 0.5f, 0.2f);
            graphics.drawRectangle(pos, size, color);
        }
    }

    // Draw terrain
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float height = getTerrainHeight(x, y);
            float resource = getResourceAmount(x, y);
            
            // Calculate color based on height and resources
            glm::vec4 color(0.0f, 0.5f + height * 0.5f, 0.0f, 1.0f);
            if (resource > 0.0f) {
                color.r = resource;
            }
            
            // Draw terrain tile
            graphics.drawRectangle(
                glm::vec2(x * 10.0f, y * 10.0f),
                glm::vec2(10.0f, 10.0f),
                color
            );
        }
    }
    
    // Render entities
    entityManager.render(graphics);
}

void World::generateTerrain() {
    // Generate terrain using Perlin noise
    generatePerlinNoise(terrain, width, height, 0.1f);
    
    // Generate resources
    generatePerlinNoise(resources, width, height, 0.05f);
    
    // Normalize and threshold resources
    for (float& resource : resources) {
        resource = std::max(0.0f, resource - 0.5f) * 2.0f;
    }
}

void World::generatePerlinNoise(std::vector<float>& noise, int width, int height, float scale) {
    // Simple noise generation (placeholder for proper Perlin noise)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float nx = x * scale;
            float ny = y * scale;
            
            // Simple noise function
            float value = std::sin(nx) * std::cos(ny);
            value = (value + 1.0f) * 0.5f; // Normalize to [0, 1]
            
            noise[y * width + x] = value;
        }
    }
}

float World::getTerrainHeight(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 0.0f;
    }
    return terrain[y * width + x];
}

float World::getResourceAmount(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 0.0f;
    }
    return resources[y * width + x];
}

void World::removeEntity(size_t index) {
    entityManager.removeEntity(index);
} 