#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <sstream> // Use sstream instead of fmt
#include "../../src/VectorRenderer.h"
#include "../../src/VectorGraphics.h"
#include "../TestUtils.h"
#include "../Mocks/MockGL.h"
#include <memory>
#include <vector>

using namespace Rendering;

// Mock VectorRenderer for testing without actual OpenGL
class MockVectorRenderer {
public:
    MockVectorRenderer() {
        // Initialize with default settings
    }

    // Simulates drawing a rectangle
    void drawRect(float x, float y, float width, float height, const glm::vec4& color) {
        // Each rectangle is 2 triangles = 6 vertices
        MockOpenGL::incrementDrawCalls();
        MockOpenGL::addVertices(6);
        
        // For tracking shader changes based on color
        if (m_lastColor != color) {
            MockOpenGL::updateUniform("color");
            m_lastColor = color;
        }
    }

    // Simulates drawing a line
    void drawLine(float x1, float y1, float x2, float y2, float thickness, const glm::vec4& color) {
        // Each line has 2 triangles = 6 vertices
        MockOpenGL::incrementDrawCalls();
        MockOpenGL::addVertices(6);
        
        // Update shader uniforms if color changes
        if (m_lastColor != color) {
            MockOpenGL::updateUniform("color");
            m_lastColor = color;
        }
    }

    // Simulates drawing a circle
    void drawCircle(float x, float y, float radius, const glm::vec4& color, int segments = 36) {
        // Each circle is drawn with triangles from center to edge
        // vertices = segments + 2 (center + starting point repeated at end)
        MockOpenGL::incrementDrawCalls();
        MockOpenGL::addVertices(segments + 2);
        
        // Update shader uniforms if color changes
        if (m_lastColor != color) {
            MockOpenGL::updateUniform("color");
            m_lastColor = color;
        }
    }

    // Simulates drawing a polygon
    void drawPolygon(const std::vector<glm::vec2>& points, const glm::vec4& color, bool filled = true) {
        int vertexCount = 0;
        
        if (filled) {
            // For a filled polygon, we make triangles by fanning from first point
            // This simple approach works for convex polygons
            // Each triangle has 3 vertices
            vertexCount = (points.size() - 2) * 3;
        } else {
            // For an outline, we connect each point with a line segment
            // Each line segment has 2 vertices
            vertexCount = points.size() * 2;
        }
        
        MockOpenGL::incrementDrawCalls();
        MockOpenGL::addVertices(vertexCount);
        
        // Update shader uniforms if color changes
        if (m_lastColor != color) {
            MockOpenGL::updateUniform("color");
            m_lastColor = color;
        }
    }

    // Batch rendering simulation
    void beginBatch() {
        m_batchActive = true;
        m_batchVertices = 0;
        m_batchColorChanges = 0;
    }

    void endBatch() {
        if (m_batchActive) {
            // One draw call for the entire batch
            if (m_batchVertices > 0) {
                MockOpenGL::incrementDrawCalls();
                MockOpenGL::addVertices(m_batchVertices);
            }
            
            // Add state changes for different colors in batch
            for (int i = 0; i < m_batchColorChanges; i++) {
                MockOpenGL::updateUniform("color");
            }
            
            m_batchActive = false;
        }
    }

    void batchRect(float x, float y, float width, float height, const glm::vec4& color) {
        m_batchVertices += 6;  // 2 triangles
        
        // Track color changes in batch
        if (m_lastColor != color) {
            m_batchColorChanges++;
            m_lastColor = color;
        }
    }

    void batchLine(float x1, float y1, float x2, float y2, float thickness, const glm::vec4& color) {
        m_batchVertices += 6;  // 2 triangles
        
        // Track color changes in batch
        if (m_lastColor != color) {
            m_batchColorChanges++;
            m_lastColor = color;
        }
    }

    void batchCircle(float x, float y, float radius, const glm::vec4& color, int segments = 36) {
        m_batchVertices += segments + 2;
        
        // Track color changes in batch
        if (m_lastColor != color) {
            m_batchColorChanges++;
            m_lastColor = color;
        }
    }

private:
    glm::vec4 m_lastColor{0.0f, 0.0f, 0.0f, 1.0f};
    bool m_batchActive = false;
    int m_batchVertices = 0;
    int m_batchColorChanges = 0;
};

// Helper function to generate random colors
glm::vec4 randomColor() {
    static int colorIndex = 0;
    const glm::vec4 colors[] = {
        {1.0f, 0.0f, 0.0f, 1.0f},  // Red
        {0.0f, 1.0f, 0.0f, 1.0f},  // Green
        {0.0f, 0.0f, 1.0f, 1.0f},  // Blue
        {1.0f, 1.0f, 0.0f, 1.0f},  // Yellow
        {1.0f, 0.0f, 1.0f, 1.0f},  // Magenta
        {0.0f, 1.0f, 1.0f, 1.0f},  // Cyan
        {0.5f, 0.5f, 0.5f, 1.0f},  // Gray
        {1.0f, 0.5f, 0.0f, 1.0f},  // Orange
        {0.5f, 0.0f, 0.5f, 1.0f},  // Purple
        {0.0f, 0.5f, 0.0f, 1.0f}   // Dark Green
    };
    
    return colors[(colorIndex++) % 10];
}

// Helper function to create benchmark labels with string concatenation instead of fmt
std::string makeLabel(const std::string& prefix, int count, const std::string& suffix) {
    std::ostringstream ss;
    ss << prefix << " " << count << " " << suffix;
    return ss.str();
}

TEST_CASE("Vector renderer basic tests", "[rendering][vector]") {
    MockVectorRenderer renderer;
    
    SECTION("Draw calls create expected GL operations") {
        MockOpenGL::resetCounters();
        
        // Draw a rectangle
        renderer.drawRect(10, 10, 100, 50, {1.0f, 0.0f, 0.0f, 1.0f});
        
        REQUIRE(MockOpenGL::getDrawCallCount() == 1);
        REQUIRE(MockOpenGL::getVertexCount() == 6);
        REQUIRE(MockOpenGL::getStateChangeCount() == 1); // First color set
        
        // Draw another rectangle with same color
        renderer.drawRect(120, 10, 100, 50, {1.0f, 0.0f, 0.0f, 1.0f});
        
        REQUIRE(MockOpenGL::getDrawCallCount() == 2);
        REQUIRE(MockOpenGL::getVertexCount() == 12);
        REQUIRE(MockOpenGL::getStateChangeCount() == 1); // No color change
        
        // Draw with different color
        renderer.drawRect(230, 10, 100, 50, {0.0f, 1.0f, 0.0f, 1.0f});
        
        REQUIRE(MockOpenGL::getDrawCallCount() == 3);
        REQUIRE(MockOpenGL::getVertexCount() == 18);
        REQUIRE(MockOpenGL::getStateChangeCount() == 2); // Color changed
    }
    
    SECTION("Batch rendering reduces draw calls") {
        MockOpenGL::resetCounters();
        
        // Without batching - 3 rectangles = 3 draw calls
        renderer.drawRect(10, 10, 100, 50, {1.0f, 0.0f, 0.0f, 1.0f});
        renderer.drawRect(120, 10, 100, 50, {0.0f, 1.0f, 0.0f, 1.0f});
        renderer.drawRect(230, 10, 100, 50, {0.0f, 0.0f, 1.0f, 1.0f});
        
        int nonBatchedDrawCalls = MockOpenGL::getDrawCallCount();
        int nonBatchedStateChanges = MockOpenGL::getStateChangeCount();
        
        // With batching - 3 rectangles = 1 draw call with multiple state changes
        MockOpenGL::resetCounters();
        
        renderer.beginBatch();
        renderer.batchRect(10, 10, 100, 50, {1.0f, 0.0f, 0.0f, 1.0f});
        renderer.batchRect(120, 10, 100, 50, {0.0f, 1.0f, 0.0f, 1.0f});
        renderer.batchRect(230, 10, 100, 50, {0.0f, 0.0f, 1.0f, 1.0f});
        renderer.endBatch();
        
        int batchedDrawCalls = MockOpenGL::getDrawCallCount();
        int batchedStateChanges = MockOpenGL::getStateChangeCount();
        
        // Check batching reduced draw calls
        REQUIRE(batchedDrawCalls < nonBatchedDrawCalls);
        REQUIRE(batchedDrawCalls == 1);
        
        // Color changes should still happen
        REQUIRE(batchedStateChanges == 3);
    }
}

TEST_CASE("Vector renderer performance benchmarks", "[benchmark][rendering][vector]") {
    MockVectorRenderer renderer;
    
    SECTION("Rectangles - individual vs batched") {
        std::vector<int> counts = {10, 100, 1000, 10000};
        
        for (int count : counts) {
            // Individual drawing
            BENCHMARK(makeLabel("Draw", count, "individual rectangles")) {
                MockOpenGL::resetCounters();
                
                for (int i = 0; i < count; i++) {
                    float x = static_cast<float>(i % 100) * 10.0f;
                    float y = static_cast<float>(i / 100) * 10.0f;
                    glm::vec4 color = randomColor();
                    
                    renderer.drawRect(x, y, 8.0f, 8.0f, color);
                }
                
                return MockOpenGL::getDrawCallCount();
            };
            
            // Batched drawing
            BENCHMARK(makeLabel("Draw", count, "batched rectangles")) {
                MockOpenGL::resetCounters();
                
                renderer.beginBatch();
                for (int i = 0; i < count; i++) {
                    float x = static_cast<float>(i % 100) * 10.0f;
                    float y = static_cast<float>(i / 100) * 10.0f;
                    glm::vec4 color = randomColor();
                    
                    renderer.batchRect(x, y, 8.0f, 8.0f, color);
                }
                renderer.endBatch();
                
                return MockOpenGL::getDrawCallCount();
            };
            
            // Verify the actual counts match expectations
            MockOpenGL::resetCounters();
            
            // Individual drawing
            for (int i = 0; i < count; i++) {
                float x = static_cast<float>(i % 100) * 10.0f;
                float y = static_cast<float>(i / 100) * 10.0f;
                glm::vec4 color = randomColor();
                
                renderer.drawRect(x, y, 8.0f, 8.0f, color);
            }
            
            int individualDrawCalls = MockOpenGL::getDrawCallCount();
            REQUIRE(individualDrawCalls == count);
            
            // Batched drawing
            MockOpenGL::resetCounters();
            
            renderer.beginBatch();
            for (int i = 0; i < count; i++) {
                float x = static_cast<float>(i % 100) * 10.0f;
                float y = static_cast<float>(i / 100) * 10.0f;
                glm::vec4 color = randomColor();
                
                renderer.batchRect(x, y, 8.0f, 8.0f, color);
            }
            renderer.endBatch();
            
            int batchedDrawCalls = MockOpenGL::getDrawCallCount();
            REQUIRE(batchedDrawCalls == 1);
        }
    }
    
    SECTION("Circles - individual vs batched") {
        std::vector<int> counts = {10, 100, 1000};
        
        for (int count : counts) {
            // Individual drawing
            BENCHMARK(makeLabel("Draw", count, "individual circles")) {
                MockOpenGL::resetCounters();
                
                for (int i = 0; i < count; i++) {
                    float x = static_cast<float>(i % 100) * 10.0f;
                    float y = static_cast<float>(i / 100) * 10.0f;
                    float radius = 4.0f;
                    glm::vec4 color = randomColor();
                    
                    // Vary segments based on radius (not needed in mock but realistic)
                    int segments = 16; // Fixed for benchmark consistency
                    renderer.drawCircle(x, y, radius, color, segments);
                }
                
                return MockOpenGL::getDrawCallCount();
            };
            
            // Batched drawing
            BENCHMARK(makeLabel("Draw", count, "batched circles")) {
                MockOpenGL::resetCounters();
                
                renderer.beginBatch();
                for (int i = 0; i < count; i++) {
                    float x = static_cast<float>(i % 100) * 10.0f;
                    float y = static_cast<float>(i / 100) * 10.0f;
                    float radius = 4.0f;
                    glm::vec4 color = randomColor();
                    
                    // Vary segments based on radius (not needed in mock but realistic)
                    int segments = 16; // Fixed for benchmark consistency
                    renderer.batchCircle(x, y, radius, color, segments);
                }
                renderer.endBatch();
                
                return MockOpenGL::getDrawCallCount();
            };
        }
    }
    
    SECTION("Lines - individual vs batched") {
        std::vector<int> counts = {10, 100, 1000, 10000};
        
        for (int count : counts) {
            // Individual drawing
            BENCHMARK(makeLabel("Draw", count, "individual lines")) {
                MockOpenGL::resetCounters();
                
                for (int i = 0; i < count; i++) {
                    float x1 = static_cast<float>(i % 100) * 10.0f;
                    float y1 = static_cast<float>(i / 100) * 10.0f;
                    float x2 = x1 + 10.0f;
                    float y2 = y1 + 10.0f;
                    float thickness = 1.0f;
                    glm::vec4 color = randomColor();
                    
                    renderer.drawLine(x1, y1, x2, y2, thickness, color);
                }
                
                return MockOpenGL::getDrawCallCount();
            };
            
            // Batched drawing
            BENCHMARK(makeLabel("Draw", count, "batched lines")) {
                MockOpenGL::resetCounters();
                
                renderer.beginBatch();
                for (int i = 0; i < count; i++) {
                    float x1 = static_cast<float>(i % 100) * 10.0f;
                    float y1 = static_cast<float>(i / 100) * 10.0f;
                    float x2 = x1 + 10.0f;
                    float y2 = y1 + 10.0f;
                    float thickness = 1.0f;
                    glm::vec4 color = randomColor();
                    
                    renderer.batchLine(x1, y1, x2, y2, thickness, color);
                }
                renderer.endBatch();
                
                return MockOpenGL::getDrawCallCount();
            };
        }
    }
}

TEST_CASE("Vector graphics complex scene test", "[benchmark][rendering][vector]") {
    MockVectorRenderer renderer;
    
    SECTION("UI Panel Rendering") {
        BENCHMARK("Render complex UI panel non-batched") {
            MockOpenGL::resetCounters();
            
            // Background panel
            renderer.drawRect(50, 50, 400, 300, {0.2f, 0.2f, 0.2f, 0.8f});
            
            // Title bar
            renderer.drawRect(50, 50, 400, 30, {0.3f, 0.3f, 0.5f, 1.0f});
            
            // Close button
            renderer.drawRect(420, 55, 20, 20, {0.8f, 0.2f, 0.2f, 1.0f});
            
            // Content area outline
            renderer.drawRect(60, 90, 380, 250, {0.3f, 0.3f, 0.3f, 1.0f});
            
            // Content items - some boxes and separators
            for (int i = 0; i < 5; i++) {
                float y = 100.0f + i * 50.0f;
                
                // Item background
                renderer.drawRect(70, y, 360, 40, {0.25f, 0.25f, 0.25f, 1.0f});
                
                // Item icon (circle)
                renderer.drawCircle(90, y + 20, 15, {0.5f, 0.8f, 0.2f, 1.0f});
                
                // Item text background
                renderer.drawRect(120, y + 10, 200, 20, {0.4f, 0.4f, 0.4f, 1.0f});
                
                // Item checkbox
                renderer.drawRect(340, y + 10, 20, 20, {0.6f, 0.6f, 0.6f, 1.0f});
                
                // Item checkbox mark (if checked)
                if (i % 2 == 0) {
                    renderer.drawLine(343, y + 13, 357, y + 27, 2.0f, {0.2f, 0.8f, 0.2f, 1.0f});
                    renderer.drawLine(343, y + 27, 357, y + 13, 2.0f, {0.2f, 0.8f, 0.2f, 1.0f});
                }
            }
            
            // Footer
            renderer.drawRect(50, 350, 400, 30, {0.3f, 0.3f, 0.5f, 1.0f});
            
            // OK button
            renderer.drawRect(370, 355, 60, 20, {0.2f, 0.6f, 0.2f, 1.0f});
            
            // Cancel button
            renderer.drawRect(300, 355, 60, 20, {0.6f, 0.2f, 0.2f, 1.0f});
            
            return MockOpenGL::getDrawCallCount();
        };
        
        BENCHMARK("Render complex UI panel batched") {
            MockOpenGL::resetCounters();
            
            renderer.beginBatch();
            
            // Background panel
            renderer.batchRect(50, 50, 400, 300, {0.2f, 0.2f, 0.2f, 0.8f});
            
            // Title bar
            renderer.batchRect(50, 50, 400, 30, {0.3f, 0.3f, 0.5f, 1.0f});
            
            // Close button
            renderer.batchRect(420, 55, 20, 20, {0.8f, 0.2f, 0.2f, 1.0f});
            
            // Content area outline
            renderer.batchRect(60, 90, 380, 250, {0.3f, 0.3f, 0.3f, 1.0f});
            
            // Content items - some boxes and separators
            for (int i = 0; i < 5; i++) {
                float y = 100.0f + i * 50.0f;
                
                // Item background
                renderer.batchRect(70, y, 360, 40, {0.25f, 0.25f, 0.25f, 1.0f});
                
                // Item icon (circle)
                renderer.batchCircle(90, y + 20, 15, {0.5f, 0.8f, 0.2f, 1.0f});
                
                // Item text background
                renderer.batchRect(120, y + 10, 200, 20, {0.4f, 0.4f, 0.4f, 1.0f});
                
                // Item checkbox
                renderer.batchRect(340, y + 10, 20, 20, {0.6f, 0.6f, 0.6f, 1.0f});
                
                // Item checkbox mark (if checked)
                if (i % 2 == 0) {
                    renderer.batchLine(343, y + 13, 357, y + 27, 2.0f, {0.2f, 0.8f, 0.2f, 1.0f});
                    renderer.batchLine(343, y + 27, 357, y + 13, 2.0f, {0.2f, 0.8f, 0.2f, 1.0f});
                }
            }
            
            // Footer
            renderer.batchRect(50, 350, 400, 30, {0.3f, 0.3f, 0.5f, 1.0f});
            
            // OK button
            renderer.batchRect(370, 355, 60, 20, {0.2f, 0.6f, 0.2f, 1.0f});
            
            // Cancel button
            renderer.batchRect(300, 355, 60, 20, {0.6f, 0.2f, 0.2f, 1.0f});
            
            renderer.endBatch();
            
            return MockOpenGL::getDrawCallCount();
        };
        
        // Validate actual reduction in draw calls
        MockOpenGL::resetCounters();
        
        // Non-batched rendering (count the operations)
        // Background panel
        renderer.drawRect(50, 50, 400, 300, {0.2f, 0.2f, 0.2f, 0.8f});
        renderer.drawRect(50, 50, 400, 30, {0.3f, 0.3f, 0.5f, 1.0f});
        renderer.drawRect(420, 55, 20, 20, {0.8f, 0.2f, 0.2f, 1.0f});
        renderer.drawRect(60, 90, 380, 250, {0.3f, 0.3f, 0.3f, 1.0f});
        
        for (int i = 0; i < 5; i++) {
            float y = 100.0f + i * 50.0f;
            renderer.drawRect(70, y, 360, 40, {0.25f, 0.25f, 0.25f, 1.0f});
            renderer.drawCircle(90, y + 20, 15, {0.5f, 0.8f, 0.2f, 1.0f});
            renderer.drawRect(120, y + 10, 200, 20, {0.4f, 0.4f, 0.4f, 1.0f});
            renderer.drawRect(340, y + 10, 20, 20, {0.6f, 0.6f, 0.6f, 1.0f});
            
            if (i % 2 == 0) {
                renderer.drawLine(343, y + 13, 357, y + 27, 2.0f, {0.2f, 0.8f, 0.2f, 1.0f});
                renderer.drawLine(343, y + 27, 357, y + 13, 2.0f, {0.2f, 0.8f, 0.2f, 1.0f});
            }
        }
        
        renderer.drawRect(50, 350, 400, 30, {0.3f, 0.3f, 0.5f, 1.0f});
        renderer.drawRect(370, 355, 60, 20, {0.2f, 0.6f, 0.2f, 1.0f});
        renderer.drawRect(300, 355, 60, 20, {0.6f, 0.2f, 0.2f, 1.0f});
        
        int nonBatchedDrawCalls = MockOpenGL::getDrawCallCount();
        int nonBatchedStateChanges = MockOpenGL::getStateChangeCount();
        
        // Batched rendering
        MockOpenGL::resetCounters();
        
        renderer.beginBatch();
        renderer.batchRect(50, 50, 400, 300, {0.2f, 0.2f, 0.2f, 0.8f});
        renderer.batchRect(50, 50, 400, 30, {0.3f, 0.3f, 0.5f, 1.0f});
        renderer.batchRect(420, 55, 20, 20, {0.8f, 0.2f, 0.2f, 1.0f});
        renderer.batchRect(60, 90, 380, 250, {0.3f, 0.3f, 0.3f, 1.0f});
        
        for (int i = 0; i < 5; i++) {
            float y = 100.0f + i * 50.0f;
            renderer.batchRect(70, y, 360, 40, {0.25f, 0.25f, 0.25f, 1.0f});
            renderer.batchCircle(90, y + 20, 15, {0.5f, 0.8f, 0.2f, 1.0f});
            renderer.batchRect(120, y + 10, 200, 20, {0.4f, 0.4f, 0.4f, 1.0f});
            renderer.batchRect(340, y + 10, 20, 20, {0.6f, 0.6f, 0.6f, 1.0f});
            
            if (i % 2 == 0) {
                renderer.batchLine(343, y + 13, 357, y + 27, 2.0f, {0.2f, 0.8f, 0.2f, 1.0f});
                renderer.batchLine(343, y + 27, 357, y + 13, 2.0f, {0.2f, 0.8f, 0.2f, 1.0f});
            }
        }
        
        renderer.batchRect(50, 350, 400, 30, {0.3f, 0.3f, 0.5f, 1.0f});
        renderer.batchRect(370, 355, 60, 20, {0.2f, 0.6f, 0.2f, 1.0f});
        renderer.batchRect(300, 355, 60, 20, {0.6f, 0.2f, 0.2f, 1.0f});
        renderer.endBatch();
        
        int batchedDrawCalls = MockOpenGL::getDrawCallCount();
        
        // Verify batching is effective
        INFO(makeLabel("Non-batched UI:", nonBatchedDrawCalls, "draw calls, Batched UI: " + std::to_string(batchedDrawCalls) + " draw calls"));
        REQUIRE(batchedDrawCalls < nonBatchedDrawCalls);
        REQUIRE(batchedDrawCalls == 1);
    }
}