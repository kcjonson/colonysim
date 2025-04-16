#include <catch.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <random>

#include "../TestUtils.h"
#include "../Mocks/MockGL.h"

// Simple function to check if a tile is visible within camera bounds
bool isTileVisible(const glm::vec2& tilePos, float tileSize, const glm::vec4& bounds) {
    return tilePos.x + tileSize >= bounds.x && tilePos.x - tileSize <= bounds.y && 
           tilePos.y + tileSize >= bounds.z && tilePos.y - tileSize <= bounds.w;
}

// Optimized function that does early rejection
bool isTileVisibleOptimized(const glm::vec2& tilePos, float tileSize, const glm::vec4& bounds) {
    // Quick rejection tests
    if (tilePos.x + tileSize < bounds.x || tilePos.x - tileSize > bounds.y) return false;
    if (tilePos.y + tileSize < bounds.z || tilePos.y - tileSize > bounds.w) return false;
    return true;
}

TEST_CASE("Tile visibility determination", "[benchmark][culling]") {
    MockCamera camera;
    camera.setPosition({500, 500, 100});
    
    // Camera view bounds (left, right, bottom, top)
    glm::vec4 bounds = {400, 600, 400, 600};
    
    // Create 10000 random tiles
    auto tiles = createRandomTiles(10000, 1000, 1000);
    
    BENCHMARK("Basic visibility check for 10000 tiles") {
        int visibleCount = 0;
        for (const auto& tilePos : tiles) {
            if (isTileVisible(tilePos, 10.0f, bounds)) {
                visibleCount++;
            }
        }
        return visibleCount;
    };
    
    BENCHMARK("Optimized visibility check for 10000 tiles") {
        int visibleCount = 0;
        for (const auto& tilePos : tiles) {
            if (isTileVisibleOptimized(tilePos, 10.0f, bounds)) {
                visibleCount++;
            }
        }
        return visibleCount;
    };
}

TEST_CASE("Spatial partitioning for culling", "[benchmark][culling]") {
    // Create a grid-based spatial partition
    const int WORLD_SIZE = 1000;
    const int CELL_SIZE = 100;
    const int GRID_SIZE = WORLD_SIZE / CELL_SIZE;
    
    // Camera view bounds (left, right, bottom, top)
    glm::vec4 bounds = {400, 600, 400, 600};
    
    // Create 10000 random tiles and assign them to grid cells
    auto tiles = createRandomTiles(10000, WORLD_SIZE, WORLD_SIZE);
    std::vector<std::vector<glm::vec2>> grid(GRID_SIZE * GRID_SIZE);
    
    for (const auto& tile : tiles) {
        int cellX = static_cast<int>(tile.x) / CELL_SIZE;
        int cellY = static_cast<int>(tile.y) / CELL_SIZE;
        if (cellX >= 0 && cellX < GRID_SIZE && cellY >= 0 && cellY < GRID_SIZE) {
            grid[cellY * GRID_SIZE + cellX].push_back(tile);
        }
    }
    
    BENCHMARK("Brute-force culling") {
        int visibleCount = 0;
        for (const auto& tile : tiles) {
            if (isTileVisibleOptimized(tile, 10.0f, bounds)) {
                visibleCount++;
            }
        }
        return visibleCount;
    };
    
    BENCHMARK("Grid-based spatial partitioning") {
        int visibleCount = 0;
        
        // Determine which grid cells intersect with the view bounds
        int minCellX = std::max(0, static_cast<int>(bounds.x) / CELL_SIZE);
        int maxCellX = std::min(GRID_SIZE - 1, static_cast<int>(bounds.y) / CELL_SIZE);
        int minCellY = std::max(0, static_cast<int>(bounds.z) / CELL_SIZE);
        int maxCellY = std::min(GRID_SIZE - 1, static_cast<int>(bounds.w) / CELL_SIZE);
        
        // Only check tiles in cells that intersect the view
        for (int y = minCellY; y <= maxCellY; ++y) {
            for (int x = minCellX; x <= maxCellX; ++x) {
                for (const auto& tile : grid[y * GRID_SIZE + x]) {
                    if (isTileVisibleOptimized(tile, 10.0f, bounds)) {
                        visibleCount++;
                    }
                }
            }
        }
        
        return visibleCount;
    };
}

TEST_CASE("Frustum culling effectiveness", "[benchmark][culling]") {
    // Create different camera frustums (narrow to wide)
    std::vector<glm::vec4> frustums = {
        {490, 510, 490, 510},  // Very narrow
        {450, 550, 450, 550},  // Medium
        {400, 600, 400, 600},  // Wide
        {200, 800, 200, 800},  // Very wide
        {0, 1000, 0, 1000}     // Entire world
    };
    
    auto tiles = createRandomTiles(10000, 1000, 1000);
    
    for (const auto& frustum : frustums) {
        std::string desc = "Frustum: [" +
            std::to_string(static_cast<int>(frustum.y - frustum.x)) +
            "x" +
            std::to_string(static_cast<int>(frustum.w - frustum.z)) +
            "]";
            
        // Use std::move to fix rvalue reference issue with BENCHMARK
        BENCHMARK(std::move(desc)) {
            int visibleCount = 0;
            for (const auto& tile : tiles) {
                if (isTileVisibleOptimized(tile, 10.0f, frustum)) {
                    visibleCount++;
                }
            }
            return visibleCount;
        };
    }
}