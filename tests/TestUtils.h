#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/noise.hpp>
#include "../src/Screens/WorldGen/Core/TerrainTypes.h" // Include for WorldGen::TileCoord and WorldGen::TerrainData

// Utility for measuring execution time
class TimingHelper {
public:
    static double measureExecutionTime(const std::function<void()>& func);
};

// Structure to hold render metrics
struct RenderMetrics {
    float frameTimeMs{0.0f};
    int drawCallCount{0};
    int vertexCount{0};
    int stateChangeCount{0};
    size_t memoryUsageBytes{0};
    
    void reset();
    std::string toString() const;
};

// Memory tracking utility
class MemoryTracker {
private:
    static size_t s_allocatedBytes;
    static size_t s_allocationCount;
    
public:
    static void reset();
    static void recordAllocation(size_t bytes);
    static void recordDeallocation(size_t bytes);
    static size_t getAllocatedBytes();
    static size_t getAllocationCount();
};

// Create a vector of random tile positions for testing
std::vector<glm::vec2> createRandomTiles(int count, float maxX, float maxY);

// Utility for generating test terrain data
namespace TestData {
    // Use WorldGen::TileCoord and WorldGen::TerrainData directly
    std::unordered_map<WorldGen::TileCoord, WorldGen::TerrainData> 
    generateTestTerrain(int width, int height);
}

// Macro to track rendering metrics for a function call
#define TRACK_RENDER_METRICS(func, metrics) \
    do { \
        MemoryTracker::reset(); \
        MockOpenGL::resetCounters(); \
        auto start = std::chrono::high_resolution_clock::now(); \
        func; \
        auto end = std::chrono::high_resolution_clock::now(); \
        metrics.frameTimeMs = std::chrono::duration<float, std::milli>(end - start).count(); \
        metrics.drawCallCount = MockOpenGL::getDrawCallCount(); \
        metrics.vertexCount = MockOpenGL::getVertexCount(); \
        metrics.stateChangeCount = MockOpenGL::getStateChangeCount(); \
        metrics.memoryUsageBytes = MemoryTracker::getAllocatedBytes(); \
    } while(0)