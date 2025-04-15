#include "TerrainGenerator.h"
#include <iostream>
#include <random>
#include <cmath>
#include <algorithm>

namespace WorldGen {

// Improved noise2D with gradient noise
float TerrainGenerator::noise2D(float x, float y, unsigned int seed) {
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
float TerrainGenerator::fbm(float x, float y, int octaves, float persistence, unsigned int seed) {
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

// Hash the string seed to a numeric value
unsigned int TerrainGenerator::getHashedSeed(const std::string& seed) {
    if (seed.empty()) return 0; // Default seed if empty

    // Simple hash function (FNV-1a)
    unsigned int hash = 2166136261u;
    for (char c : seed) {
        hash ^= static_cast<unsigned int>(c);
        hash *= 16777619u;
    }
    return hash;
}

void TerrainGenerator::generateTerrain(
    std::unordered_map<std::pair<int, int>, TerrainData>& terrainData,
    int generateDistance,
    unsigned int hashedSeed
) {
    std::cout << "Generating terrain..." << std::endl;
    terrainData.clear();

    for (int y = -generateDistance; y <= generateDistance; y++) {
        for (int x = -generateDistance; x <= generateDistance; x++) {
            float nx = x * 0.05f;
            float ny = y * 0.05f;

            // Pass hashedSeed to fbm
            float heightValue = fbm(nx, ny, 4, 0.5f, hashedSeed);
            heightValue = (heightValue + 1.0f) * 0.5f;
            float resourceValue = fbm(nx * 0.5f, ny * 0.5f, 4, 0.5f, hashedSeed);

            TerrainData data;
            data.height = heightValue;
            data.resource = resourceValue;

            // Smooth terrain transitions
            if (heightValue > 0.7f) {
                data.type = 2; // Mountain
                data.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
            } 
            else if (heightValue > 0.5f) {
                data.type = 1; // Land
                float blend = (heightValue - 0.5f) / 0.2f;
                data.color = glm::mix(
                    glm::vec4(0.0f, 0.5f, 0.0f, 1.0f),
                    glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
                    blend
                );
            }
            else {
                data.type = 0; // Water
                float blend = heightValue / 0.5f;
                data.color = glm::mix(
                    glm::vec4(0.0f, 0.2f, 0.5f, 1.0f),
                    glm::vec4(0.0f, 0.5f, 0.8f, 1.0f),
                    blend
                );
            }

            // Resource tint
            if (resourceValue > 0.5f) {
                data.color.r = resourceValue;
            }

            terrainData[{x, y}] = data;
        }
    }
    
    /* 
    TODO: Generate a spherical world
    - divide the sphere into a grid of hexagons
    - generate major world features in the hexagons
    - each hexagon will have a number of world features (e.g. it can have a mountain, river and forest)


    TODO: generate sectors of terrain
    - each sector should be a hexagon
    - sectors may have a number of influence regions around their edge
    - the influence regions will be expressed as a side (top, bottom, left, right) and a start/end distance from the edge vector start
    - the influence points will be used to connect features between sectors (like rivers)

    TODO: generate the tiles within each sector
    - the tiles will be generated respecting the edge influence regions
    */
}

} // namespace WorldGen