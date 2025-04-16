#include <catch.hpp>
#include <vector>
#include <memory>
#include <chrono>
#include <algorithm>
#include <random>
#include <glm/glm.hpp>
#include "../TestUtils.h"
#include "../Mocks/MockGL.h"

// Test cases for tile rendering performance
class QuadTree {
public:
    QuadTree(const glm::vec4& bounds, int maxDepth = 5, int maxObjectsPerNode = 10)
        : m_bounds(bounds), m_maxDepth(maxDepth), m_maxObjectsPerNode(maxObjectsPerNode), m_depth(0) {
        m_children.reserve(4);
    }

    // Insert a tile position into the quadtree
    void insert(const glm::vec2& position) {
        // If we have children, insert into them
        if (!m_children.empty()) {
            int index = getQuadrantIndex(position);
            if (index != -1) {
                m_children[index]->insert(position);
                return;
            }
        }

        // Otherwise add to this node
        m_objects.push_back(position);

        // Check if we need to split
        if (m_objects.size() > m_maxObjectsPerNode && m_depth < m_maxDepth) {
            split();
            
            // Redistribute objects to children
            auto objectsCopy = m_objects;
            m_objects.clear();
            
            for (const auto& obj : objectsCopy) {
                int index = getQuadrantIndex(obj);
                if (index != -1) {
                    m_children[index]->insert(obj);
                } else {
                    m_objects.push_back(obj); // Keep in this node if it doesn't fit in a child
                }
            }
        }
    }

    // Query tiles within a given boundary
    std::vector<glm::vec2> query(const glm::vec4& queryBounds) const {
        std::vector<glm::vec2> result;
        
        // Skip this node if it doesn't overlap the query bounds
        if (!overlaps(queryBounds)) {
            return result;
        }
        
        // Add objects from this node that are within bounds
        for (const auto& obj : m_objects) {
            if (containsPoint(queryBounds, obj)) {
                result.push_back(obj);
            }
        }
        
        // Check children
        for (const auto& child : m_children) {
            auto childResults = child->query(queryBounds);
            result.insert(result.end(), childResults.begin(), childResults.end());
        }
        
        return result;
    }

private:
    glm::vec4 m_bounds; // x, y, width, height
    int m_maxDepth;
    int m_maxObjectsPerNode;
    int m_depth;
    std::vector<glm::vec2> m_objects;
    std::vector<std::shared_ptr<QuadTree>> m_children;

    // Split this node into four children
    void split() {
        float x = m_bounds.x;
        float y = m_bounds.y;
        float halfWidth = m_bounds.z * 0.5f;
        float halfHeight = m_bounds.w * 0.5f;

        // Create four children representing the quadrants
        m_children.push_back(std::make_shared<QuadTree>(
            glm::vec4(x, y, halfWidth, halfHeight), 
            m_maxDepth, m_maxObjectsPerNode));
        m_children.back()->m_depth = m_depth + 1;

        m_children.push_back(std::make_shared<QuadTree>(
            glm::vec4(x + halfWidth, y, halfWidth, halfHeight), 
            m_maxDepth, m_maxObjectsPerNode));
        m_children.back()->m_depth = m_depth + 1;

        m_children.push_back(std::make_shared<QuadTree>(
            glm::vec4(x, y + halfHeight, halfWidth, halfHeight),
            m_maxDepth, m_maxObjectsPerNode));
        m_children.back()->m_depth = m_depth + 1;

        m_children.push_back(std::make_shared<QuadTree>(
            glm::vec4(x + halfWidth, y + halfHeight, halfWidth, halfHeight),
            m_maxDepth, m_maxObjectsPerNode));
        m_children.back()->m_depth = m_depth + 1;
    }

    // Get the index of the child that contains the position (-1 if none)
    int getQuadrantIndex(const glm::vec2& position) const {
        if (m_children.empty()) return -1;

        float midX = m_bounds.x + m_bounds.z * 0.5f;
        float midY = m_bounds.y + m_bounds.w * 0.5f;

        bool right = position.x >= midX;
        bool bottom = position.y >= midY;

        if (!right && !bottom) return 0; // Top-left
        if (right && !bottom) return 1;  // Top-right
        if (!right && bottom) return 2;  // Bottom-left
        return 3;                        // Bottom-right
    }

    // Check if this node's bounds overlap with the query bounds
    bool overlaps(const glm::vec4& queryBounds) const {
        // queryBounds: (left, right, bottom, top)
        // m_bounds: (x, y, width, height)
        return !(
            m_bounds.x > queryBounds.y ||                // this is to the right of query
            m_bounds.x + m_bounds.z < queryBounds.x ||   // this is to the left of query
            m_bounds.y > queryBounds.w ||                // this is below query
            m_bounds.y + m_bounds.w < queryBounds.z      // this is above query
        );
    }

    // Check if a point is contained within bounds
    bool containsPoint(const glm::vec4& bounds, const glm::vec2& point) const {
        // bounds: (left, right, bottom, top)
        return (
            point.x >= bounds.x &&
            point.x <= bounds.y &&
            point.y >= bounds.z &&
            point.y <= bounds.w
        );
    }
};

// Create a vector of tile positions for testing
std::vector<glm::vec2> createTileGrid(int width, int height, float tileSize = 1.0f) {
    std::vector<glm::vec2> tiles;
    tiles.reserve(width * height);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            tiles.emplace_back(x * tileSize, y * tileSize);
        }
    }
    
    return tiles;
}

TEST_CASE("Basic tile visibility", "[rendering][tile]") {
    // Create a 100x100 tile grid
    std::vector<glm::vec2> tiles = createTileGrid(100, 100, 10.0f);
    
    // Define view bounds for different camera positions
    // Format: (left, right, bottom, top)
    std::vector<glm::vec4> viewBounds = {
        {0.0f, 200.0f, 0.0f, 200.0f},     // Small view near origin
        {250.0f, 450.0f, 250.0f, 450.0f}, // View in the middle
        {900.0f, 1100.0f, 900.0f, 1100.0f} // View near the edge
    };
    
    // Test basic visibility checks
    SECTION("Simple visibility check") {
        const float tileSize = 10.0f;
        
        for (const auto& bounds : viewBounds) {
            int visibleCount = 0;
            
            // Check visibility for each tile
            auto start = std::chrono::high_resolution_clock::now();
            
            for (const auto& tile : tiles) {
                if (tile.x + tileSize >= bounds.x &&  // right edge > left bound
                    tile.x <= bounds.y &&             // left edge < right bound
                    tile.y + tileSize >= bounds.z &&  // bottom edge > bottom bound
                    tile.y <= bounds.w) {             // top edge < top bound
                    visibleCount++;
                }
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            float duration = std::chrono::duration<float, std::milli>(end - start).count();
            
            INFO("View bounds: (" << bounds.x << ", " << bounds.y << ", " << bounds.z << ", " << bounds.w << ")");
            INFO("Visible tiles: " << visibleCount << " / " << tiles.size());
            INFO("Visibility check time: " << duration << " ms");
            
            // Ensure that we're properly detecting visible tiles
            REQUIRE(visibleCount < tiles.size());
        }
    }
}

TEST_CASE("Tile visibility performance", "[benchmark][rendering][tile]") {
    // Create a 1000x1000 tile grid for a larger test
    std::vector<glm::vec2> tiles = createTileGrid(1000, 1000, 10.0f);
    const float tileSize = 10.0f;
    
    // Create a quadtree for spatial partitioning
    glm::vec4 worldBounds(0.0f, 0.0f, 1000.0f * tileSize, 1000.0f * tileSize);
    auto quadtree = std::make_shared<QuadTree>(worldBounds);
    
    // Insert all tiles into the quadtree
    for (const auto& tile : tiles) {
        quadtree->insert(tile);
    }
    
    // Define test cases with different view sizes
    std::vector<glm::vec4> testViews = {
        {1000.0f, 2000.0f, 1000.0f, 2000.0f}, // 100x100 tiles view
        {2000.0f, 2200.0f, 2000.0f, 2200.0f}, // 20x20 tiles view
        {5000.0f, 5050.0f, 5000.0f, 5050.0f}  // 5x5 tiles view
    };
    
    for (const auto& view : testViews) {
        std::string viewSize = std::to_string(static_cast<int>((view.y - view.x) / tileSize)) + 
                              "x" + 
                              std::to_string(static_cast<int>((view.w - view.z) / tileSize));
        
        BENCHMARK("Brute force visibility check (" + viewSize + " tiles)") {
            int visibleCount = 0;
            
            for (const auto& tile : tiles) {
                if (tile.x + tileSize >= view.x && 
                    tile.x <= view.y && 
                    tile.y + tileSize >= view.z && 
                    tile.y <= view.w) {
                    visibleCount++;
                }
            }
            
            return visibleCount;
        };
        
        BENCHMARK("Quadtree visibility check (" + viewSize + " tiles)") {
            auto visibleTiles = quadtree->query(view);
            return visibleTiles.size();
        };
    }
}

TEST_CASE("Frustum culling performance", "[benchmark][rendering][tile]") {
    // Create a grid of different sizes
    const std::vector<int> worldSizes = {100, 250, 500, 1000};
    const float tileSize = 10.0f;
    
    // Test the effect of different frustum sizes on culling efficiency
    for (int size : worldSizes) {
        std::string worldDesc = std::to_string(size) + "x" + std::to_string(size);
        auto tiles = createTileGrid(size, size, tileSize);
        
        // Test with different view sizes
        std::vector<float> viewPercentages = {0.1f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float viewPct : viewPercentages) {
            float viewSize = size * tileSize * viewPct;
            float centerPos = size * tileSize * 0.5f;
            
            // Define view bounds centered in the world
            glm::vec4 viewBounds(
                centerPos - viewSize * 0.5f,
                centerPos + viewSize * 0.5f,
                centerPos - viewSize * 0.5f,
                centerPos + viewSize * 0.5f
            );
            
            std::string testName = "Frustum culling - " + worldDesc + 
                                  " world, " + std::to_string(static_cast<int>(viewPct * 100)) + "% view";
            
            BENCHMARK(testName) {
                MockOpenGL::resetCounters();
                
                int visibleTiles = 0;
                for (const auto& tile : tiles) {
                    bool visible = (
                        tile.x + tileSize >= viewBounds.x && 
                        tile.x <= viewBounds.y && 
                        tile.y + tileSize >= viewBounds.z && 
                        tile.y <= viewBounds.w
                    );
                    
                    if (visible) {
                        visibleTiles++;
                        MockOpenGL::drawElements(1, 6, 0, nullptr);
                    }
                }
                
                return visibleTiles;
            };
        }
    }
}

TEST_CASE("Tile batch rendering performance", "[benchmark][rendering][tile]") {
    const int worldSize = 100;
    float tileSize = 10.0f;
    auto tiles = createTileGrid(worldSize, worldSize, tileSize);
    
    // Define a view that sees 25% of the world
    float halfViewSize = worldSize * tileSize * 0.25f;
    float centerPos = worldSize * tileSize * 0.5f;
    glm::vec4 viewBounds(
        centerPos - halfViewSize,
        centerPos + halfViewSize,
        centerPos - halfViewSize,
        centerPos + halfViewSize
    );
    
    BENCHMARK("No batching - one draw call per visible tile") {
        MockOpenGL::resetCounters();
        
        int visibleTiles = 0;
        for (const auto& tile : tiles) {
            bool visible = (
                tile.x + tileSize >= viewBounds.x && 
                tile.x <= viewBounds.y && 
                tile.y + tileSize >= viewBounds.z && 
                tile.y <= viewBounds.w
            );
            
            if (visible) {
                visibleTiles++;
                MockOpenGL::drawElements(1, 6, 0, nullptr);
                
                // Simulate binding a texture for each tile
                MockOpenGL::bindTexture(1, static_cast<int>(tile.x + tile.y) % 5);
            }
        }
        
        return visibleTiles;
    };
    
    BENCHMARK("Simple batching - group by texture") {
        MockOpenGL::resetCounters();
        
        // First collect visible tiles
        std::vector<glm::vec2> visibleTiles;
        for (const auto& tile : tiles) {
            bool visible = (
                tile.x + tileSize >= viewBounds.x && 
                tile.x <= viewBounds.y && 
                tile.y + tileSize >= viewBounds.z && 
                tile.y <= viewBounds.w
            );
            
            if (visible) {
                visibleTiles.push_back(tile);
            }
        }
        
        // Group by texture (simulate 5 different textures based on position)
        std::vector<std::vector<glm::vec2>> textureGroups(5);
        
        for (const auto& tile : visibleTiles) {
            int textureId = static_cast<int>(tile.x + tile.y) % 5;
            textureGroups[textureId].push_back(tile);
        }
        
        // Render each texture group with a single texture bind
        for (int i = 0; i < 5; i++) {
            if (!textureGroups[i].empty()) {
                MockOpenGL::bindTexture(1, i);
                
                // Batch rendering for this texture group
                for (const auto& tile : textureGroups[i]) {
                    MockOpenGL::drawElements(1, 6, 0, nullptr);
                }
            }
        }
        
        return visibleTiles.size();
    };
    
    BENCHMARK("Advanced batching - merge tiles into larger draws") {
        MockOpenGL::resetCounters();
        
        // First collect visible tiles into a 2D grid for neighbor checking
        bool visibilityGrid[worldSize][worldSize] = {false};
        int visibleCount = 0;
        
        for (const auto& tile : tiles) {
            int x = static_cast<int>(tile.x / tileSize);
            int y = static_cast<int>(tile.y / tileSize);
            
            bool visible = (
                tile.x + tileSize >= viewBounds.x && 
                tile.x <= viewBounds.y && 
                tile.y + tileSize >= viewBounds.z && 
                tile.y <= viewBounds.w
            );
            
            if (visible && x < worldSize && y < worldSize) {
                visibilityGrid[y][x] = true;
                visibleCount++;
            }
        }
        
        // Find continuous horizontal spans to batch
        for (int y = 0; y < worldSize; y++) {
            int spanStart = -1;
            int textureId = -1;
            
            for (int x = 0; x < worldSize; x++) {
                if (!visibilityGrid[y][x]) {
                    if (spanStart != -1) {
                        // End of a span
                        int spanLength = x - spanStart;
                        if (spanLength > 0) {
                            // One draw call for the entire span
                            MockOpenGL::bindTexture(1, textureId);
                            MockOpenGL::drawElements(1, spanLength * 6, 0, nullptr);
                        }
                        spanStart = -1;
                    }
                    continue;
                }
                
                // Current tile's texture
                int currentTexture = static_cast<int>((x + y) * tileSize) % 5;
                
                if (spanStart == -1 || currentTexture != textureId) {
                    // End previous span if there was one
                    if (spanStart != -1) {
                        int spanLength = x - spanStart;
                        if (spanLength > 0) {
                            MockOpenGL::bindTexture(1, textureId);
                            MockOpenGL::drawElements(1, spanLength * 6, 0, nullptr);
                        }
                    }
                    
                    // Start a new span
                    spanStart = x;
                    textureId = currentTexture;
                }
            }
            
            // Handle the last span in the row
            if (spanStart != -1) {
                int spanLength = worldSize - spanStart;
                if (spanLength > 0) {
                    MockOpenGL::bindTexture(1, textureId);
                    MockOpenGL::drawElements(1, spanLength * 6, 0, nullptr);
                }
            }
        }
        
        return visibleCount;
    };
}