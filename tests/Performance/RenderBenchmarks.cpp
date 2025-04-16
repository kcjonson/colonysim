#include <catch.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <vector>

#include "../TestUtils.h"
#include "../Mocks/MockGL.h"

TEST_CASE("Basic render benchmark", "[benchmark][rendering]") {
    BENCHMARK("Render 10x10 world") {
        TestWorld world(10, 10);
        world.render();
        return MockOpenGL::getDrawCallCount();
    };
    
    BENCHMARK("Render 50x50 world") {
        TestWorld world(50, 50);
        world.render();
        return MockOpenGL::getDrawCallCount();
    };
    
    BENCHMARK("Render 100x100 world") {
        TestWorld world(100, 100);
        world.render();
        return MockOpenGL::getDrawCallCount();
    };
}

TEST_CASE("Layer sorting performance", "[benchmark][rendering]") {
    // Create a layer with many children
    std::vector<int> zIndices;
    const int CHILD_COUNT = 1000;
    
    // Initialize with random z-indices
    for (int i = 0; i < CHILD_COUNT; i++) {
        zIndices.push_back(rand());
    }
    
    BENCHMARK("Sort 1000 layers by z-index") {
        std::sort(zIndices.begin(), zIndices.end());
        return zIndices.size(); // To prevent optimization
    };
    
    // Reset with more randomly distributed values
    zIndices.clear();
    for (int i = 0; i < CHILD_COUNT; i++) {
        zIndices.push_back(rand() % 10); // Many layers share the same z-index
    }
    
    BENCHMARK("Sort 1000 layers with duplicated z-indices") {
        std::stable_sort(zIndices.begin(), zIndices.end());
        return zIndices.size(); // To prevent optimization
    };
}

TEST_CASE("Matrix caching effectiveness", "[benchmark][rendering]") {
    MockCamera camera;
    
    BENCHMARK("Uncached matrix calculations") {
        // Force recalculation every time
        camera.setPosition({rand() % 1000, rand() % 1000, 100});
        return camera.getViewMatrix() * camera.getProjectionMatrix();
    };
    
    BENCHMARK("Cached matrix with same position") {
        // Position doesn't change, should use cache
        return camera.getViewMatrix() * camera.getProjectionMatrix();
    };
    
    BENCHMARK("Uncached matrix with minor position change") {
        // Small movements should still force recalculation
        static float offset = 0.0f;
        offset += 0.1f;
        camera.setPosition({500.0f + offset, 500.0f, 100.0f});
        return camera.getViewMatrix() * camera.getProjectionMatrix();
    };
}

TEST_CASE("Draw call tracking", "[benchmark][rendering]") {
    MockOpenGL::resetCounters();
    
    SECTION("Different world sizes") {
        for (int worldSize : {10, 50, 100, 250}) {
            DYNAMIC_SECTION("World size: " << worldSize << "x" << worldSize) {
                TestWorld world(worldSize, worldSize);
                
                // Reset counter
                MockOpenGL::resetCounters();
                
                // Render the test world
                world.render();
                
                // Report metrics
                INFO("Draw calls: " << MockOpenGL::getDrawCallCount());
                INFO("Vertices: " << MockOpenGL::getVertexCount());
                INFO("State changes: " << MockOpenGL::getStateChangeCount());
                
                // Basic verification
                REQUIRE(MockOpenGL::getDrawCallCount() > 0);
            }
        }
    }
}