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

// Improved noise2D with gradient noise
float noise2D(float x, float y, unsigned int seed) {
    // Grid cell coordinates
    int x0 = static_cast<int>(std::floor(x));
    int x1 = x0 + 1;
    int y0 = static_cast<int>(std::floor(y));
    int y1 = y0 + 1;

    // Fractional parts
    float fx = x - x0;
    float fy = y - y0;

    // Smoothstep interpolation
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);

    // Random gradients (pseudo-random but deterministic)
    auto random = [](int xi, int yi, unsigned int seed) {
        float val = std::sin(xi * 12.9898f + yi * 78.233f + seed) * 43758.5453f;
        return val - std::floor(val);
    };

    // Dot products with gradients
    float n00 = random(x0, y0, seed) * 2.0f - 1.0f;
    float n01 = random(x0, y1, seed) * 2.0f - 1.0f;
    float n10 = random(x1, y0, seed) * 2.0f - 1.0f;
    float n11 = random(x1, y1, seed) * 2.0f - 1.0f;

    // Bilinear interpolation
    float nx0 = n00 + fx * (n10 - n00);
    float nx1 = n01 + fx * (n11 - n01);
    return nx0 + fy * (nx1 - nx0);
}

// Fractional Brownian Motion for terrain generation
float fbm(float x, float y, int octaves, float persistence, unsigned int seed) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        total += noise2D(x * frequency, y * frequency, seed) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= 2.0f;
    }
 
    return total / maxValue;
}

World::World(const std::string& seed) : width(100), height(100), seed(seed) {
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
    
    // Get camera position (convert to 2D if using orthographic projection)
    glm::vec3 cameraPos = camera.getPosition();
    
    return glm::vec4(
        cameraPos.x - viewWidth/2.0f,   // left
        cameraPos.x + viewWidth/2.0f,   // right
        cameraPos.y - viewHeight/2.0f,  // bottom
        cameraPos.y + viewHeight/2.0f   // top
    );
}

void World::render(VectorGraphics& graphics) {
    // Draw centerpoint for debugging
    graphics.drawCircle(glm::vec2(0.0f, 0.0f), 1.0f, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    // Get camera bounds
    glm::vec4 bounds = getCameraBounds();
    float tileSize = 20.0f; // I feel like this should be somewhere else.
    
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
    unsigned int hashedSeed = getHashedSeed(); // Get numeric hash
    tiles.clear();

    for (int y = -generateDistance; y <= generateDistance; y++) {
        for (int x = -generateDistance; x <= generateDistance; x++) {
            float nx = x * 0.05f;
            float ny = y * 0.05f;

            // Pass hashedSeed to fbm
            float heightValue = fbm(nx, ny, 4, 0.5f, hashedSeed);
            heightValue = (heightValue + 1.0f) * 0.5f;
            float resourceValue = fbm(nx * 0.5f, ny * 0.5f, 4, 0.5f, hashedSeed);

            Tile tile;
            tile.height = heightValue;
            tile.resource = resourceValue;

            // Smooth terrain transitions
            if (heightValue > 0.7f) {
                tile.type = 2; // Mountain
                tile.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
            } 
            else if (heightValue > 0.5f) {
                tile.type = 1; // Land
                float blend = (heightValue - 0.5f) / 0.2f;
                tile.color = glm::mix(
                    glm::vec4(0.0f, 0.5f, 0.0f, 1.0f),
                    glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
                    blend
                );
            }
            else {
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

// Hash the string seed to a numeric value
unsigned int World::getHashedSeed() const {
    if (seed.empty()) return 0; // Default seed if empty

    // Simple hash function (FNV-1a)
    unsigned int hash = 2166136261u;
    for (char c : seed) {
        hash ^= static_cast<unsigned int>(c);
        hash *= 16777619u;
    }
    return hash;
} 