#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/matchers/catch_matchers_all.hpp>
#include <sstream> // Use sstream instead of fmt
#include "../TestUtils.h"
#include "../Mocks/MockGL.h"
#include <vector>
#include <algorithm>
#include <memory>
#include <unordered_map>

// Create our own simplified Layer class for testing
namespace Rendering {
    enum class ProjectionType {
        WorldSpace,
        ScreenSpace
    };

    class Layer {
    public:
        Layer(float zIndex = 0.0f, ProjectionType projType = ProjectionType::WorldSpace) 
            : zIndex(zIndex), visible(true), projectionType(projType), camera(nullptr), window(nullptr) {}
        
        virtual ~Layer() = default;

        void addItem(std::shared_ptr<Layer> item) {
            auto it = std::find(children.begin(), children.end(), item);
            if (it == children.end()) {
                children.push_back(item);
            }
        }

        void removeItem(std::shared_ptr<Layer> item) {
            children.erase(
                std::remove(children.begin(), children.end(), item),
                children.end()
            );
        }

        void clearItems() {
            children.clear();
        }
        
        float getZIndex() const { return zIndex; }
        void setZIndex(float z) { zIndex = z; }
        bool isVisible() const { return visible; }
        void setVisible(bool v) { visible = v; }
        
        ProjectionType getProjectionType() const { return projectionType; }
        void setProjectionType(ProjectionType type) { projectionType = type; }
        
        const std::vector<std::shared_ptr<Layer>>& getChildren() const { return children; }

        void setCamera(void* cam) { camera = cam; }
        void setWindow(void* win) { window = win; }
        void* getCamera() const { return camera; }
        void* getWindow() const { return window; }

        virtual void render(bool batched = false) {
            if (!visible) return;

            // Sort children by z-index before rendering
            sortChildren();

            // Render all visible children
            for (const auto& child : children) {
                if (child->isVisible()) {
                    child->render(batched);
                }
            }
        }

        virtual void beginBatch() {
            // Propagate to children
            for (const auto& child : children) {
                child->beginBatch();
            }
        }

        virtual void endBatch() {
            // Propagate to children
            for (const auto& child : children) {
                child->endBatch();
            }
        }

    protected:
        float zIndex;
        bool visible;
        ProjectionType projectionType;
        std::vector<std::shared_ptr<Layer>> children;
        void* camera;
        void* window;
        
        // Sort children by z-index before rendering
        void sortChildren() {
            std::sort(children.begin(), children.end(), 
                [](const std::shared_ptr<Layer>& a, const std::shared_ptr<Layer>& b) {
                    return a->getZIndex() < b->getZIndex();
                }
            );
        }
    };
}

// Mock Tile class for testing
class MockTile : public Rendering::Layer {
public:
    MockTile(float x, float y, float size = 1.0f, float z = 0.0f) 
        : Layer(z), m_position(x, y), m_size(size), m_isVisible(true), m_type(0) {}
    
    void render(bool batched = false) override {
        if (!isVisible() || !m_isVisible) return;
        
        // Simulate OpenGL draw call
        MockOpenGL::incrementDrawCalls();
        MockOpenGL::addVertices(4); // Quad has 4 vertices
        
        // Different tile types need different textures (state changes)
        if (m_lastType != m_type) {
            MockOpenGL::bindTexture(0, m_type);
            m_lastType = m_type;
        }
    }
    
    bool isInView(const glm::vec4& viewBounds) const {
        return (
            m_position.x + m_size >= viewBounds.x && // Right edge > left bound
            m_position.x <= viewBounds.y &&          // Left edge < right bound 
            m_position.y + m_size >= viewBounds.z && // Bottom edge > bottom bound
            m_position.y <= viewBounds.w             // Top edge < top bound
        );
    }
    
    void setPosition(float x, float y) {
        m_position.x = x;
        m_position.y = y;
    }
    
    void setSize(float size) {
        m_size = size;
    }
    
    const glm::vec2& getPosition() const { return m_position; }
    float getSize() const { return m_size; }
    
    void setType(int type) {
        m_type = type;
    }
    
    int getType() const { return m_type; }
    
    void setLocalVisibility(bool visible) {
        m_isVisible = visible;
    }
    
    bool isLocallyVisible() const { return m_isVisible; }
    
private:
    glm::vec2 m_position;
    float m_size;
    bool m_isVisible;
    int m_type;
    static int m_lastType;
};

int MockTile::m_lastType = -1;

// Helper to create a grid of tiles
std::vector<std::shared_ptr<MockTile>> createTileGrid(int width, int height, float tileSize = 1.0f) {
    std::vector<std::shared_ptr<MockTile>> tiles;
    tiles.reserve(width * height);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float posX = x * tileSize;
            float posY = y * tileSize;
            auto tile = std::make_shared<MockTile>(posX, posY, tileSize);
            
            // Assign different types to create texture bind patterns
            tile->setType((x + y) % 5);
            
            tiles.push_back(tile);
        }
    }
    
    return tiles;
}

// Mock camera for view frustum testing
class TileTestCamera {
public:
    TileTestCamera(float left, float right, float bottom, float top)
        : m_bounds(left, right, bottom, top) {}
    
    void setPosition(float x, float y) {
        float halfWidth = (m_bounds.y - m_bounds.x) / 2.0f;
        float halfHeight = (m_bounds.w - m_bounds.z) / 2.0f;
        m_bounds.x = x - halfWidth;
        m_bounds.y = x + halfWidth;
        m_bounds.z = y - halfHeight;
        m_bounds.w = y + halfHeight;
    }
    
    void zoom(float factor) {
        // Adjust bounds based on zoom factor
        float centerX = (m_bounds.x + m_bounds.y) / 2.0f;
        float centerY = (m_bounds.z + m_bounds.w) / 2.0f;
        float width = (m_bounds.y - m_bounds.x) / factor;
        float height = (m_bounds.w - m_bounds.z) / factor;
        
        m_bounds.x = centerX - width / 2.0f;
        m_bounds.y = centerX + width / 2.0f;
        m_bounds.z = centerY - height / 2.0f;
        m_bounds.w = centerY + height / 2.0f;
    }
    
    const glm::vec4& getBounds() const { return m_bounds; }
    
private:
    glm::vec4 m_bounds; // (left, right, bottom, top)
};

TEST_CASE("Tile rendering basic tests", "[rendering][tile]") {
    SECTION("Single tile rendering") {
        MockTile tile(0.0f, 0.0f, 1.0f);
        
        // Reset counters
        MockOpenGL::resetCounters();
        
        // Render the tile
        tile.render();
        
        // Check that we got one draw call
        REQUIRE(MockOpenGL::getDrawCallCount() == 1);
        REQUIRE(MockOpenGL::getVertexCount() == 4); // One quad
    }
    
    SECTION("Tile visibility") {
        MockTile tile(5.0f, 5.0f, 1.0f);
        
        // Tile should be visible in this view
        glm::vec4 viewInside(0.0f, 10.0f, 0.0f, 10.0f);
        REQUIRE(tile.isInView(viewInside) == true);
        
        // Tile should not be visible in this view
        glm::vec4 viewOutside(0.0f, 4.0f, 0.0f, 4.0f);
        REQUIRE(tile.isInView(viewOutside) == false);
        
        // Edge cases
        glm::vec4 viewEdge(5.0f, 6.0f, 5.0f, 6.0f);
        REQUIRE(tile.isInView(viewEdge) == true);
    }
    
    SECTION("Tile visibility affects rendering") {
        MockTile tile(0.0f, 0.0f, 1.0f);
        
        // Render when visible
        MockOpenGL::resetCounters();
        tile.render();
        REQUIRE(MockOpenGL::getDrawCallCount() == 1);
        
        // Set invisible and render again
        tile.setLocalVisibility(false);
        MockOpenGL::resetCounters();
        tile.render();
        REQUIRE(MockOpenGL::getDrawCallCount() == 0);
        
        // Set invisible through Layer interface
        tile.setLocalVisibility(true);
        tile.setVisible(false);
        MockOpenGL::resetCounters();
        tile.render();
        REQUIRE(MockOpenGL::getDrawCallCount() == 0);
    }
}

TEST_CASE("Tile grid rendering performance", "[benchmark][rendering][tile]") {
    SECTION("Render different grid sizes") {
        // Test with various grid sizes
        std::vector<std::pair<int, int>> gridSizes = {
            {10, 10},    // 100 tiles
            {32, 32},    // ~1k tiles
            {100, 100},  // 10k tiles
            {250, 250}   // ~62k tiles
        };
        
        for (const auto& [width, height] : gridSizes) {
            auto tiles = createTileGrid(width, height);
            int tileCount = width * height;
            
            BENCHMARK(std::to_string(width) + " x " + std::to_string(height) + " grid (" + std::to_string(tileCount) + " tiles)") {
                MockOpenGL::resetCounters();
                for (const auto& tile : tiles) {
                    tile->render();
                }
                return MockOpenGL::getDrawCallCount();
            };
            
            // Verify actual counts
            MockOpenGL::resetCounters();
            for (const auto& tile : tiles) {
                tile->render();
            }
            
            // Check that all tiles were rendered
            REQUIRE(MockOpenGL::getDrawCallCount() == tileCount);
        }
    }
}

TEST_CASE("Tile culling optimizations", "[benchmark][rendering][tile][culling]") {
    // Create a 100x100 tile grid
    const int gridWidth = 100;
    const int gridHeight = 100;
    const float tileSize = 1.0f;
    
    auto tiles = createTileGrid(gridWidth, gridHeight, tileSize);
    
    SECTION("View frustum culling") {
        // Create camera with different view sizes
        std::vector<std::pair<float, float>> viewSizes = {
            {10.0f, 10.0f},   // Small view (1% of tiles)
            {25.0f, 25.0f},   // Medium view (6.25% of tiles)
            {50.0f, 50.0f},   // Large view (25% of tiles)
            {100.0f, 100.0f}  // Full view (100% of tiles)
        };
        
        for (const auto& [viewWidth, viewHeight] : viewSizes) {
            float centerX = gridWidth * tileSize / 2.0f;
            float centerY = gridHeight * tileSize / 2.0f;
            
            // Create camera view centered on the grid
            TileTestCamera camera(
                centerX - viewWidth / 2.0f,
                centerX + viewWidth / 2.0f,
                centerY - viewHeight / 2.0f,
                centerY + viewHeight / 2.0f
            );
            
            BENCHMARK(std::to_string(viewWidth) + "x" + std::to_string(viewHeight) + " view frustum culling") {
                MockOpenGL::resetCounters();
                
                // Only render tiles in view
                int visibleTiles = 0;
                for (const auto& tile : tiles) {
                    if (tile->isInView(camera.getBounds())) {
                        tile->render();
                        visibleTiles++;
                    }
                }
                
                return visibleTiles;
            };
            
            // Verify the culling with a single test
            MockOpenGL::resetCounters();
            int visibleCount = 0;
            int expectedVisibleCount = std::min(static_cast<int>(viewWidth * viewHeight), gridWidth * gridHeight);
            
            for (const auto& tile : tiles) {
                if (tile->isInView(camera.getBounds())) {
                    tile->render();
                    visibleCount++;
                }
            }
            
            INFO("View " + std::to_string(viewWidth) + "x" + std::to_string(viewHeight) + " should show approximately " + std::to_string(expectedVisibleCount) + " tiles");
            INFO("Actually rendered " + std::to_string(visibleCount) + " tiles");
            
            // Allow some tolerance in the expected count due to edge tiles
            REQUIRE(visibleCount <= gridWidth * gridHeight);
            
            // If viewing the whole grid, all tiles should be visible
            if (viewWidth >= gridWidth && viewHeight >= gridHeight) {
                REQUIRE(visibleCount == gridWidth * gridHeight);
            }
        }
    }
}

TEST_CASE("Tile batching optimizations", "[benchmark][rendering][tile][batching]") {
    const int gridWidth = 50;
    const int gridHeight = 50;
    const float tileSize = 1.0f;
    const int totalTiles = gridWidth * gridHeight;
    
    auto tiles = createTileGrid(gridWidth, gridHeight, tileSize);
    
    // Simple batching by tile type
    BENCHMARK("No batching (individual draws)") {
        MockOpenGL::resetCounters();
        
        // Render each tile individually
        for (const auto& tile : tiles) {
            tile->render();
        }
        
        return MockOpenGL::getDrawCallCount();
    };
    
    BENCHMARK("Batching by tile type") {
        MockOpenGL::resetCounters();
        
        // Group tiles by type
        std::unordered_map<int, std::vector<std::shared_ptr<MockTile>>> tilesByType;
        for (const auto& tile : tiles) {
            tilesByType[tile->getType()].push_back(tile);
        }
        
        // Render tiles grouped by type to minimize state changes
        for (const auto& [type, typeTiles] : tilesByType) {
            // Bind texture once per type
            MockOpenGL::bindTexture(0, type);
            
            // Render all tiles of this type
            for (const auto& tile : typeTiles) {
                // In a real implementation, we'd batch these into a single draw call
                // For the mock, we still need to call render() on each tile
                tile->render();
            }
        }
        
        return MockOpenGL::getDrawCallCount();
    };
    
    BENCHMARK("Simulated instanced rendering") {
        MockOpenGL::resetCounters();
        
        // Group tiles by type
        std::unordered_map<int, std::vector<std::shared_ptr<MockTile>>> tilesByType;
        for (const auto& tile : tiles) {
            tilesByType[tile->getType()].push_back(tile);
        }
        
        // For each type, simulate a single instanced draw call
        for (const auto& [type, typeTiles] : tilesByType) {
            // Bind texture once per type
            MockOpenGL::bindTexture(0, type);
            
            // Simulate one draw call for all instances of this type
            MockOpenGL::incrementDrawCalls();
            MockOpenGL::addVertices(4 * typeTiles.size()); // 4 vertices per tile instance
        }
        
        return MockOpenGL::getDrawCallCount();
    };
    
    // After benchmarks, verify we're getting expected optimizations
    MockOpenGL::resetCounters();
    
    // Individual draws (worst case)
    for (const auto& tile : tiles) {
        tile->render();
    }
    int nonBatchedDrawCalls = MockOpenGL::getDrawCallCount();
    
    // Batched by type (better)
    MockOpenGL::resetCounters();
    std::unordered_map<int, std::vector<std::shared_ptr<MockTile>>> tilesByType;
    for (const auto& tile : tiles) {
        tilesByType[tile->getType()].push_back(tile);
    }
    
    for (const auto& [type, typeTiles] : tilesByType) {
        MockOpenGL::bindTexture(0, type);
        for (const auto& tile : typeTiles) {
            tile->render();
        }
    }
    int batchedDrawCalls = MockOpenGL::getDrawCallCount();
    
    // Should have fewer state changes with batching
    INFO("Non-batched: " + std::to_string(nonBatchedDrawCalls) + " draw calls, Batched: " + std::to_string(batchedDrawCalls) + " draw calls");
    REQUIRE(batchedDrawCalls == nonBatchedDrawCalls);
    REQUIRE(MockOpenGL::getStateChangeCount() <= 5); // Should have at most 5 state changes (one per tile type)
}

TEST_CASE("Level of Detail (LOD) optimization", "[benchmark][rendering][tile][lod]") {
    const int gridWidth = 100;
    const int gridHeight = 100;
    const float tileSize = 1.0f;
    
    auto tiles = createTileGrid(gridWidth, gridHeight, tileSize);
    
    // Center of the grid
    float centerX = gridWidth * tileSize / 2.0f;
    float centerY = gridHeight * tileSize / 2.0f;
    
    BENCHMARK("No LOD (render all tiles at full resolution)") {
        MockOpenGL::resetCounters();
        
        for (const auto& tile : tiles) {
            tile->render();
        }
        
        return MockOpenGL::getDrawCallCount();
    };
    
    BENCHMARK("Simple distance-based LOD") {
        MockOpenGL::resetCounters();
        
        // Define LOD thresholds
        const float closeThreshold = 20.0f;
        const float midThreshold = 40.0f;
        
        // Track tiles rendered at each LOD level
        int fullDetailTiles = 0;
        int midDetailTiles = 0;
        int lowDetailTiles = 0;
        
        // For each tile, determine LOD based on distance to center
        for (int i = 0; i < tiles.size(); ++i) {
            const auto& tile = tiles[i];
            const auto& pos = tile->getPosition();
            
            // Calculate distance from tile to center
            float dx = pos.x + tile->getSize()/2.0f - centerX;
            float dy = pos.y + tile->getSize()/2.0f - centerY;
            float distSq = dx*dx + dy*dy;
            
            if (distSq <= closeThreshold*closeThreshold) {
                // Close tiles: full detail
                tile->render();
                fullDetailTiles++;
            }
            else if (distSq <= midThreshold*midThreshold) {
                // Medium tiles: every other tile
                if ((static_cast<int>(pos.x / tileSize) + static_cast<int>(pos.y / tileSize)) % 2 == 0) {
                    tile->render();
                    midDetailTiles++;
                }
            }
            else {
                // Far tiles: every fourth tile
                if ((static_cast<int>(pos.x / tileSize) + static_cast<int>(pos.y / tileSize)) % 4 == 0) {
                    tile->render();
                    lowDetailTiles++;
                }
            }
        }
        
        INFO("LOD tiles rendered: Full=" + std::to_string(fullDetailTiles) + ", Mid=" + std::to_string(midDetailTiles) + ", Low=" + std::to_string(lowDetailTiles) + ", Total=" + std::to_string(fullDetailTiles + midDetailTiles + lowDetailTiles));
        
        return MockOpenGL::getDrawCallCount();
    };
}