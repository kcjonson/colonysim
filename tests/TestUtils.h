#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Define hash for std::pair<int, int> to use it as a key in unordered_map
namespace std {
    template<>
    struct hash<std::pair<int, int>> {
        std::size_t operator()(const std::pair<int, int>& p) const noexcept {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
        }
    };
}

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
    // Forward declarations from actual game code to make sure signatures match
    namespace WorldGen {
        struct TerrainData {
            float height;
            int type;
            glm::vec4 color;
        };
    }
    
    // Use a custom pair type or a struct to avoid std::pair hash issues
    struct TileCoord {
        int x;
        int y;
        
        bool operator==(const TileCoord& other) const {
            return x == other.x && y == other.y;
        }
    };
}

// Hash function for the TileCoord struct
namespace std {
    template <>
    struct hash<TestData::TileCoord> {
        size_t operator()(const TestData::TileCoord& coord) const {
            return std::hash<int>()(coord.x) ^ (std::hash<int>()(coord.y) << 1);
        }
    };
}

namespace TestData {
    std::unordered_map<TileCoord, WorldGen::TerrainData> 
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