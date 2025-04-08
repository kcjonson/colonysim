#include "World.h"
#include "ConfigManager.h"
#include "Camera.h"
#include "VectorGraphics.h"
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>
#include <iomanip> // For std::setw and std::setprecision
#include <unordered_map>

constexpr float PI = 3.14159265358979323846f;

// Improved noise2D function with smoother transitions
float noise2D(float x, float y) {
    // Use fractional parts for smoother interpolation
    float fx = x - std::floor(x);
    float fy = y - std::floor(y);

    // Smoothstep interpolation
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);

    // Sample noise at grid points
    float n00 = std::sin(x) * std::cos(y);
    float n01 = std::sin(x) * std::cos(y + 1.0f);
    float n10 = std::sin(x + 1.0f) * std::cos(y);
    float n11 = std::sin(x + 1.0f) * std::cos(y + 1.0f);

    // Bilinear interpolation
    float nx0 = n00 + fx * (n10 - n00);
    float nx1 = n01 + fx * (n11 - n01);
    return nx0 + fy * (nx1 - nx0);
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
    generateTerrain();
}

World::~World() = default;

void World::setTile(int x, int y, const Tile& tile) {
    tiles[{x, y}] = tile;
}

Tile World::getTile(int x, int y) const {
    auto it = tiles.find({x, y});
    if (it != tiles.end()) {
        return it->second;
    }
    return Tile{}; // Return default tile if not found
}

void World::update(float deltaTime) {
    entityManager.update(deltaTime);
}

glm::vec4 World::getCameraBounds() const {
    float viewWidth = ConfigManager::getInstance().getViewHeight() * camera.getAspectRatio();
    float viewHeight = ConfigManager::getInstance().getViewHeight();
    
    return glm::vec4(
        -viewWidth/2.0f,  // left
        viewWidth/2.0f,   // right
        -viewHeight/2.0f, // bottom
        viewHeight/2.0f   // top
    );
}

void World::render(VectorGraphics& graphics) {
    // Draw centerpoint for debugging
    graphics.drawCircle(glm::vec2(0.0f, 0.0f), 1.0f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    // Get camera bounds
    glm::vec4 bounds = getCameraBounds();
    float tileSize = 40.0f; // I feel like this should be somewhere else.
    
    // Calculate visible tile range with overscan
    int minX = static_cast<int>((bounds.x - overscanAmount * tileSize) / tileSize) - 1;
    int maxX = static_cast<int>((bounds.y + overscanAmount * tileSize) / tileSize) + 1;
    int minY = static_cast<int>((bounds.z - overscanAmount * tileSize) / tileSize) - 1;
    int maxY = static_cast<int>((bounds.w + overscanAmount * tileSize) / tileSize) + 1;

    // Draw only visible tiles
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            auto it = tiles.find({x, y});
            if (it != tiles.end()) {
                const auto& tile = it->second;
                graphics.drawRectangle(
                    glm::vec2(x * tileSize, y * tileSize),
                    glm::vec2(tileSize, tileSize),
                    tile.color
                );
            }
        }
    }

    // Render entities
    entityManager.render(graphics);
}

void World::generateTerrain() {
    std::cout << "Generating terrain..." << std::endl;
    generateTilesInRadius();
}

void World::generateTilesInRadius() {
    tiles.clear();

    for (int y = -generateDistance; y <= generateDistance; y++) {
        for (int x = -generateDistance; x <= generateDistance; x++) {
            float nx = x * 0.05f; // Adjusted noise scale
            float ny = y * 0.05f;

            // Smoother noise with FBM
            float heightValue = fbm(nx, ny, 4, 0.5f);
            float resourceValue = fbm(nx * 0.5f, ny * 0.5f, 4, 0.5f);

            Tile tile;
            tile.height = heightValue;
            tile.resource = resourceValue;

            // Smooth terrain transitions
            if (heightValue > 0.7f) {
                tile.type = 2; // Mountain
                tile.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
            } else if (heightValue > 0.5f) {
                tile.type = 1; // Land
                float blend = (heightValue - 0.5f) / 0.2f;
                tile.color = glm::mix(
                    glm::vec4(0.0f, 0.5f, 0.0f, 1.0f),
                    glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
                    blend
                );
            } else {
                tile.type = 0; // Water
                float blend = heightValue / 0.5f;
                tile.color = glm::mix(
                    glm::vec4(0.0f, 0.2f, 0.5f, 1.0f),
                    glm::vec4(0.0f, 0.5f, 0.8f, 1.0f),
                    blend
                );
            }

            // Resource tint
            if (resourceValue > 0.5f) {
                tile.color.r = resourceValue;
            }

            setTile(x, y, tile);
        }
    }
}

size_t World::createEntity(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
    return entityManager.createEntity(position, size, color);
}

Entity* World::getEntity(size_t index) {
    return entityManager.getEntity(index);
}

void World::removeEntity(size_t index) {
    entityManager.removeEntity(index);
} 