#include <catch.hpp>
#include <memory>
#include <random>
#include <vector>
#include <glm/glm.hpp>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>

#include "../TestUtils.h"
#include "../Mocks/MockGL.h"
#include "../../src/Rendering/Layer.h"

// Helper function to format the size metrics
std::string formatSize(int worldSize) {
    std::stringstream ss;
    ss << worldSize << "x" << worldSize << " (" << worldSize * worldSize << " tiles)";
    return ss.str();
}

// Helper to collect comprehensive metrics about rendering at different scales
RenderMetrics benchmarkWorldRendering(int worldSize) {
    TestWorld world(worldSize, worldSize);
    RenderMetrics metrics;
    
    // Reset counters before test
    MockOpenGL::resetCounters();
    MemoryTracker::reset();
    
    // Measure rendering time
    auto start = std::chrono::high_resolution_clock::now();
    world.render();
    auto end = std::chrono::high_resolution_clock::now();
    
    // Collect metrics
    metrics.frameTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
    metrics.drawCallCount = MockOpenGL::getDrawCallCount();
    metrics.vertexCount = MockOpenGL::getVertexCount();
    metrics.stateChangeCount = MockOpenGL::getStateChangeCount();
    metrics.memoryUsageBytes = MemoryTracker::getAllocatedBytes();
    
    return metrics;
}

TEST_CASE("Rendering performance scaling", "[benchmark][scaling]") {
    std::vector<int> worldSizes = {10, 25, 50, 100, 250, 500, 1000};
    
    SECTION("Collect full metrics for different world sizes") {
        for (int size : worldSizes) {
            DYNAMIC_SECTION("World size: " << formatSize(size)) {
                RenderMetrics metrics = benchmarkWorldRendering(size);
                
                // Output all metrics for comparison
                INFO("Frame time: " << metrics.frameTimeMs << " ms");
                INFO("Draw calls: " << metrics.drawCallCount);
                INFO("Vertices: " << metrics.vertexCount);
                INFO("State changes: " << metrics.stateChangeCount);
                INFO("Memory usage: " << (metrics.memoryUsageBytes / 1024) << " KB");
                
                // Sanity check that rendering produced something
                REQUIRE(metrics.drawCallCount > 0);
            }
        }
    }
    
    SECTION("Benchmark draw call scaling") {
        for (int size : worldSizes) {
            BENCHMARK("Draw calls for " + formatSize(size)) {
                TestWorld world(size, size);
                MockOpenGL::resetCounters();
                world.render();
                return MockOpenGL::getDrawCallCount();
            };
        }
    }
    
    SECTION("Benchmark render time scaling") {
        for (int size : worldSizes) {
            BENCHMARK("Render time for " + formatSize(size)) {
                return benchmarkWorldRendering(size).frameTimeMs;
            };
        }
    }
}

// Class for testing rendering optimizations
class RenderOptimizer {
public:
    static void renderBaseline(int numTiles) {
        // Simulate standard rendering (one draw call per tile)
        for (int i = 0; i < numTiles; i++) {
            MockOpenGL::drawElements(1, 6, 0, nullptr);
            
            // Simulate texture changes
            if (i % 10 == 0) {
                MockOpenGL::bindTexture(1, i % 5);
            }
        }
    }
    
    static void renderBatched(int numTiles) {
        // Simulate batched rendering (one draw call for many tiles)
        int batchSize = 100;
        int numBatches = (numTiles + batchSize - 1) / batchSize;
        
        for (int b = 0; b < numBatches; b++) {
            int tilesInBatch = std::min(batchSize, numTiles - b * batchSize);
            
            // One draw call per batch
            MockOpenGL::drawElements(1, tilesInBatch * 6, 0, nullptr);
        }
    }
    
    static void renderFrustumCulled(int numTiles) {
        // Simulate frustum culling (only render visible tiles)
        // Assume 50% of tiles are visible in frustum
        int visibleTiles = numTiles / 2;
        
        for (int i = 0; i < visibleTiles; i++) {
            MockOpenGL::drawElements(1, 6, 0, nullptr);
        }
    }
    
    static void renderLOD(int numTiles) {
        // Simulate level-of-detail rendering
        // Close tiles: full detail (1:1)
        // Medium tiles: half detail (4:1)
        // Far tiles: quarter detail (16:1)
        
        int closeTiles = numTiles / 5;     // 20% are close
        int mediumTiles = numTiles / 3;    // 33% are medium
        int farTiles = numTiles - closeTiles - mediumTiles;  // 47% are far
        
        // Close tiles rendered individually
        for (int i = 0; i < closeTiles; i++) {
            MockOpenGL::drawElements(1, 6, 0, nullptr);
        }
        
        // Medium tiles rendered at 4:1 ratio
        int mediumGroups = (mediumTiles + 3) / 4;
        for (int i = 0; i < mediumGroups; i++) {
            MockOpenGL::drawElements(1, 6, 0, nullptr);  // One quad for 4 tiles
        }
        
        // Far tiles rendered at 16:1 ratio
        int farGroups = (farTiles + 15) / 16;
        for (int i = 0; i < farGroups; i++) {
            MockOpenGL::drawElements(1, 6, 0, nullptr);  // One quad for 16 tiles
        }
    }
};

TEST_CASE("Render optimization techniques", "[benchmark][rendering][optimization]") {
    const int numTiles = 10000;  // Test with 10k tiles
    
    BENCHMARK("Baseline (no optimizations)") {
        MockOpenGL::resetCounters();
        RenderOptimizer::renderBaseline(numTiles);
        return MockOpenGL::getDrawCallCount();
    };
    
    BENCHMARK("Batched rendering") {
        MockOpenGL::resetCounters();
        RenderOptimizer::renderBatched(numTiles);
        return MockOpenGL::getDrawCallCount();
    };
    
    BENCHMARK("Frustum culling") {
        MockOpenGL::resetCounters();
        RenderOptimizer::renderFrustumCulled(numTiles);
        return MockOpenGL::getDrawCallCount();
    };
    
    BENCHMARK("Level of detail (LOD)") {
        MockOpenGL::resetCounters();
        RenderOptimizer::renderLOD(numTiles);
        return MockOpenGL::getDrawCallCount();
    };
}

TEST_CASE("Measuring cache coherence effects", "[benchmark][rendering][cache]") {
    const int numTiles = 1000;  // Use 1000 tiles for this test
    
    // Test different tile ordering strategies
    std::vector<std::string> orderingStrategies = {
        "row-major",      // Process tiles in row-major order (cache friendly)
        "random",         // Process tiles in random order (cache unfriendly)
        "texture-sorted"  // Process tiles sorted by texture (minimize state changes)
    };
    
    for (const auto& strategy : orderingStrategies) {
        std::vector<glm::vec2> tiles;
        
        if (strategy == "row-major") {
            // Create tiles in row-major order (cache friendly)
            int gridSize = static_cast<int>(std::ceil(std::sqrt(numTiles)));
            for (int y = 0; y < gridSize; y++) {
                for (int x = 0; x < gridSize; x++) {
                    if (tiles.size() < static_cast<size_t>(numTiles)) {
                        tiles.push_back(glm::vec2(x * 20.0f, y * 20.0f));
                    }
                }
            }
        }
        else if (strategy == "random") {
            // Create random tiles
            tiles = createRandomTiles(numTiles, 1000.0f, 1000.0f);
        }
        else if (strategy == "texture-sorted") {
            // Create tiles grouped by texture type
            int gridSize = static_cast<int>(std::ceil(std::sqrt(numTiles)));
            std::vector<glm::vec2> byTexture[3];
            
            for (int y = 0; y < gridSize; y++) {
                for (int x = 0; x < gridSize; x++) {
                    if (byTexture[0].size() + byTexture[1].size() + byTexture[2].size() < static_cast<size_t>(numTiles)) {
                        int textureId = (x + y) % 3;
                        byTexture[textureId].push_back(glm::vec2(x * 20.0f, y * 20.0f));
                    }
                }
            }
            
            // Combine texture groups
            for (int t = 0; t < 3; t++) {
                tiles.insert(tiles.end(), byTexture[t].begin(), byTexture[t].end());
            }
        }
        
        BENCHMARK("Rendering in " + strategy + " order") {
            MockOpenGL::resetCounters();
            
            // Simulate rendering tiles in the specified order
            for (size_t i = 0; i < tiles.size(); i++) {
                // Simulate drawing a tile
                MockOpenGL::drawElements(1, 6, 0, nullptr);
                
                // Simulate texture changes based on the strategy
                if (strategy == "texture-sorted") {
                    if (i > 0 && i % (numTiles / 3) == 0) {
                        MockOpenGL::bindTexture(1, static_cast<int>(i / (numTiles / 3)));
                    }
                } else if (i % 10 == 0) {
                    MockOpenGL::bindTexture(1, static_cast<int>(i % 5));
                }
            }
            
            return MockOpenGL::getStateChangeCount();
        };
    }
}