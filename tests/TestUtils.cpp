#include "TestUtils.h"
#include <cmath>
#include <chrono>
#include <sstream>
#include <iomanip>

// Initialize static members
size_t MemoryTracker::s_allocatedBytes = 0;
size_t MemoryTracker::s_allocationCount = 0;

// Implementation for TimingHelper
double TimingHelper::measureExecutionTime(const std::function<void()>& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> elapsed = end - start;
    return elapsed.count();
}

// Implementation for RenderMetrics
void RenderMetrics::reset() {
    frameTimeMs = 0.0f;
    drawCallCount = 0;
    vertexCount = 0;
    stateChangeCount = 0;
    memoryUsageBytes = 0;
}

std::string RenderMetrics::toString() const {
    std::stringstream ss;
    ss << "Frame time: " << std::fixed << std::setprecision(2) << frameTimeMs << " ms, "
       << "Draw calls: " << drawCallCount << ", "
       << "Vertices: " << vertexCount << ", "
       << "State changes: " << stateChangeCount << ", "
       << "Memory: " << (memoryUsageBytes / 1024.0f) << " KB";
    return ss.str();
}

// Implementation for MemoryTracker
void MemoryTracker::reset() {
    s_allocatedBytes = 0;
    s_allocationCount = 0;
}

void MemoryTracker::recordAllocation(size_t bytes) {
    s_allocatedBytes += bytes;
    s_allocationCount++;
}

void MemoryTracker::recordDeallocation(size_t bytes) {
    if (s_allocatedBytes >= bytes) {
        s_allocatedBytes -= bytes;
    } else {
        s_allocatedBytes = 0; // prevent underflow
    }
}

size_t MemoryTracker::getAllocatedBytes() { 
    return s_allocatedBytes; 
}

size_t MemoryTracker::getAllocationCount() { 
    return s_allocationCount; 
}

// Implementation for utility functions
std::vector<glm::vec2> createRandomTiles(int count, float maxX, float maxY) {
    std::vector<glm::vec2> tiles;
    tiles.reserve(count);
    
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> distX(0, maxX);
    std::uniform_real_distribution<float> distY(0, maxY);
    
    for (int i = 0; i < count; i++) {
        tiles.emplace_back(distX(rng), distY(rng));
    }
    
    return tiles;
}

namespace TestData {
    // Update signature and implementation to use WorldGen types
    std::unordered_map<WorldGen::TileCoord, WorldGen::TerrainData> 
    generateTestTerrain(int width, int height) {
        std::unordered_map<WorldGen::TileCoord, WorldGen::TerrainData> terrain;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                WorldGen::TileCoord coord{x, y}; // Use WorldGen::TileCoord
                
                // Calculate normalized position
                float nx = static_cast<float>(x) / width;
                float ny = static_cast<float>(y) / height;
                
                // Generate height using a simple algorithm
                float height = 0.2f + 0.3f * std::sin(nx * 5.0f) * std::cos(ny * 3.0f);
                
                // Determine terrain type based on height
                int type = (height > 0.6f) ? 2 :  // Mountains
                          (height > 0.3f) ? 1 :  // Grass
                          0;                     // Water
                
                // Assign color based on type
                glm::vec4 color;
                switch (type) {
                    case 2: color = glm::vec4(0.5f, 0.35f, 0.05f, 1.0f); break; // Brown
                    case 1: color = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f); break;  // Green
                    default: color = glm::vec4(0.0f, 0.0f, 0.8f, 1.0f); break; // Blue
                }
                
                // Store terrain data using WorldGen::TerrainData
                // Note: The original TestData::WorldGen::TerrainData only had height, type, color.
                // The actual WorldGen::TerrainData has height, resource, type, color.
                // We'll set resource to 0.0f for simplicity in this test data.
                terrain[coord] = {height, 0.0f, type, color}; // Use WorldGen::TileCoord and WorldGen::TerrainData
            }
        }
        
        return terrain;
    }
}