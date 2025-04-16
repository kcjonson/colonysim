#include <catch.hpp>
#include <algorithm>
#include <memory>
#include <vector>
#include <random>
#include <chrono>
#include <sstream>
#include <functional>

// Forward declarations
class MockLayer;

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

        void testSortChildren() { 
            sortChildren(); 
        }

        // Friend class for testing
        friend class ::MockLayer;

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

#include "../TestUtils.h"
#include "../Mocks/MockGL.h"

// Mock layer class for testing
class MockLayer : public Rendering::Layer {
public:
    explicit MockLayer(float zIndex = 0.0f, const std::string& name = "MockLayer") 
        : Layer(zIndex), m_name(name), m_renderCount(0) {}
    
    // Override render for testing
    void render(bool batched = false) override {
        if (!isVisible()) return;
        
        m_renderCount++;
        
        // Simulate rendering by incrementing mock GL counters
        MockOpenGL::drawElements(1, 6, 0, nullptr);
        
        // Render children using the base implementation
        for (const auto& child : children) {
            if (child->isVisible()) {
                child->render(batched);
            }
        }
    }
    
    // Helper methods for testing
    void exposedSortChildren() {
        testSortChildren();
    }
    
    std::vector<float> getChildrenZValues() const {
        std::vector<float> zValues;
        for (const auto& child : children) {
            zValues.push_back(child->getZIndex());
        }
        return zValues;
    }
    
    int getRenderCount() const { return m_renderCount; }
    const std::string& getName() const { return m_name; }
    
private:
    std::string m_name;
    int m_renderCount;
};

// Generate layers with random z-indices
std::vector<std::shared_ptr<MockLayer>> generateRandomLayers(int count) {
    std::vector<std::shared_ptr<MockLayer>> layers;
    layers.reserve(count);
    
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> zDist(-1000.0f, 1000.0f);
    
    for (int i = 0; i < count; i++) {
        layers.push_back(std::make_shared<MockLayer>(zDist(rng), "Layer" + std::to_string(i)));
    }
    
    return layers;
}

// Helper function to add layers to a parent layer
void addLayersToParent(Rendering::Layer& parent, const std::vector<std::shared_ptr<MockLayer>>& layers) {
    for (const auto& layer : layers) {
        parent.addItem(layer);
    }
}

TEST_CASE("Layer basic functionality", "[rendering][layer]") {
    MockLayer layer(0.0f, "TestLayer");
    
    SECTION("Add and remove items") {
        auto item1 = std::make_shared<MockLayer>(0.0f);
        auto item2 = std::make_shared<MockLayer>(1.0f);
        
        REQUIRE(layer.getChildren().size() == 0);
        layer.addItem(item1);
        REQUIRE(layer.getChildren().size() == 1);
        layer.addItem(item2);
        REQUIRE(layer.getChildren().size() == 2);
        
        // Remove items
        layer.removeItem(item1);
        REQUIRE(layer.getChildren().size() == 1);
        
        // Adding the same item twice should not increase the count
        layer.addItem(item2);
        REQUIRE(layer.getChildren().size() == 1);
    }
    
    SECTION("Layer sorting works properly") {
        auto item1 = std::make_shared<MockLayer>(5.0f);
        auto item2 = std::make_shared<MockLayer>(2.0f);
        auto item3 = std::make_shared<MockLayer>(8.0f);
        
        layer.addItem(item1);
        layer.addItem(item2);
        layer.addItem(item3);
        
        // Sort and verify order
        layer.exposedSortChildren();
        auto zValues = layer.getChildrenZValues();
        
        // Should be sorted in ascending order: 2, 5, 8
        REQUIRE(zValues.size() == 3);
        REQUIRE(std::is_sorted(zValues.begin(), zValues.end()));
        REQUIRE(zValues[0] == 2.0f);
        REQUIRE(zValues[1] == 5.0f);
        REQUIRE(zValues[2] == 8.0f);
    }
}

TEST_CASE("Layer sorting performance", "[benchmark][rendering][layer]") {
    std::vector<int> layerCounts = {10, 100, 1000, 10000};
    
    SECTION("Benchmark sorting different layer counts") {
        for (int count : layerCounts) {
            std::ostringstream ss;
            ss << "Sorting " << count << " layers";
            
            BENCHMARK(ss.str()) {
                // Create a root layer
                MockLayer root(0.0f);
                
                // Generate random child layers
                auto layers = generateRandomLayers(count);
                
                // Add layers to root
                addLayersToParent(root, layers);
                
                // Measure the sort operation
                root.testSortChildren();
                
                return layers.size();
            };
        }
    }
    
    SECTION("Benchmark rendering with layer sorting") {
        for (int count : layerCounts) {
            std::ostringstream ss;
            ss << "Rendering " << count << " sorted layers";
            
            BENCHMARK(ss.str()) {
                // Create a root layer
                MockLayer root(0.0f);
                
                // Generate random child layers
                auto layers = generateRandomLayers(count);
                
                // Add layers to root
                addLayersToParent(root, layers);
                
                // Sort and render
                MockOpenGL::resetCounters();
                root.testSortChildren();
                root.render();
                
                return MockOpenGL::getDrawCallCount();
            };
        }
    }
    
    SECTION("Compare sorted vs unsorted rendering") {
        for (int count : layerCounts) {
            if (count > 1000) continue; // Skip larger counts for this test
            
            // Create unsorted benchmark
            std::ostringstream unsortedLabel;
            unsortedLabel << "Render " << count << " unsorted layers";
            
            BENCHMARK(unsortedLabel.str()) {
                // Create a root layer
                MockLayer root(0.0f);
                
                // Generate random child layers
                auto layers = generateRandomLayers(count);
                
                // Add layers to root without sorting
                addLayersToParent(root, layers);
                
                // Render without sorting
                MockOpenGL::resetCounters();
                root.render();
                
                return MockOpenGL::getDrawCallCount();
            };
            
            // Create sorted benchmark
            std::ostringstream sortedLabel;
            sortedLabel << "Render " << count << " pre-sorted layers";
            
            BENCHMARK(sortedLabel.str()) {
                // Create a root layer
                MockLayer root(0.0f);
                
                // Generate random child layers and sort them first
                auto layers = generateRandomLayers(count);
                std::sort(layers.begin(), layers.end(), 
                    [](const std::shared_ptr<MockLayer>& a, const std::shared_ptr<MockLayer>& b) {
                        return a->getZIndex() < b->getZIndex();
                    }
                );
                
                // Add pre-sorted layers to root
                addLayersToParent(root, layers);
                
                // Render without needing to sort
                MockOpenGL::resetCounters();
                root.render();
                
                return MockOpenGL::getDrawCallCount();
            };
        }
    }
}

TEST_CASE("Layer sorting algorithms comparison", "[benchmark][rendering][layer][sorting]") {
    // Layer counts for testing
    std::vector<int> layerCounts = {100, 1000, 10000};
    
    // Generate test data sets
    std::vector<std::vector<std::shared_ptr<MockLayer>>> testData;
    
    for (int count : layerCounts) {
        testData.push_back(generateRandomLayers(count));
    }
    
    // Standard library sort
    SECTION("Compare standard sorting implementations") {
        for (size_t i = 0; i < layerCounts.size(); i++) {
            int count = layerCounts[i];
            auto& layers = testData[i];
            
            // Standard std::sort
            std::ostringstream stdSortLabel;
            stdSortLabel << count << " layers with std::sort";
            
            BENCHMARK(stdSortLabel.str()) {
                auto layersCopy = layers; // Make a copy to avoid modifying the original
                std::sort(layersCopy.begin(), layersCopy.end(), 
                    [](const std::shared_ptr<MockLayer>& a, const std::shared_ptr<MockLayer>& b) {
                        return a->getZIndex() < b->getZIndex();
                    }
                );
                return layersCopy.size();
            };
            
            // Stable sort
            std::ostringstream stableSortLabel;
            stableSortLabel << count << " layers with std::stable_sort";
            
            BENCHMARK(stableSortLabel.str()) {
                auto layersCopy = layers;
                std::stable_sort(layersCopy.begin(), layersCopy.end(), 
                    [](const std::shared_ptr<MockLayer>& a, const std::shared_ptr<MockLayer>& b) {
                        return a->getZIndex() < b->getZIndex();
                    }
                );
                return layersCopy.size();
            };
            
            // Insertion sort (for smaller data sets)
            if (count <= 1000) {
                std::ostringstream insertionSortLabel;
                insertionSortLabel << count << " layers with insertion sort";
                
                BENCHMARK(insertionSortLabel.str()) {
                    auto layersCopy = layers;
                    
                    // Simple insertion sort implementation
                    for (size_t i = 1; i < layersCopy.size(); i++) {
                        auto key = layersCopy[i];
                        int j = static_cast<int>(i) - 1;
                        
                        while (j >= 0 && layersCopy[j]->getZIndex() > key->getZIndex()) {
                            layersCopy[j + 1] = layersCopy[j];
                            j--;
                        }
                        
                        layersCopy[j + 1] = key;
                    }
                    
                    return layersCopy.size();
                };
            }
        }
    }
}

TEST_CASE("Layer tree traversal performance", "[benchmark][rendering][layer]") {
    // Test different layer tree structures
    
    SECTION("Flat layer structure vs deep hierarchy") {
        const int totalLayers = 1000;
        
        // Create a flat structure (many siblings)
        BENCHMARK("Render 1000 layers in flat structure") {
            MockLayer root(0.0f);
            auto layers = generateRandomLayers(totalLayers);
            addLayersToParent(root, layers);
            
            MockOpenGL::resetCounters();
            root.testSortChildren();
            root.render();
            
            return MockOpenGL::getDrawCallCount();
        };
        
        // Create a deep structure (many levels)
        BENCHMARK("Render 1000 layers in deep hierarchy") {
            MockLayer root(0.0f);
            
            // Create a chain of layers, each with only one child
            std::shared_ptr<MockLayer> current = std::make_shared<MockLayer>(0.0f);
            root.addItem(current);
            
            for (int i = 1; i < totalLayers; i++) {
                auto next = std::make_shared<MockLayer>(static_cast<float>(i));
                current->addItem(next);
                current = next;
            }
            
            MockOpenGL::resetCounters();
            root.testSortChildren();  // This will recursively sort all layers
            root.render();        // This will render all layers
            
            return MockOpenGL::getDrawCallCount();
        };
        
        // Create a balanced tree structure
        BENCHMARK("Render 1000 layers in balanced tree") {
            MockLayer root(0.0f);
            
            // Create approximately 10 layers of 10 children each (10^3 = 1000 total)
            int childrenPerNode = 10;
            int depth = 3;
            
            std::function<void(std::shared_ptr<MockLayer>, int, float)> buildTree;
            buildTree = [&](std::shared_ptr<MockLayer> parent, int currentDepth, float zBase) {
                if (currentDepth >= depth) return;
                
                for (int i = 0; i < childrenPerNode; i++) {
                    float z = zBase + i * 0.1f;
                    auto child = std::make_shared<MockLayer>(z);
                    parent->addItem(child);
                    
                    buildTree(child, currentDepth + 1, z);
                }
            };
            
            // Create the tree starting with the root
            auto rootNode = std::make_shared<MockLayer>(0.0f);
            root.addItem(rootNode);
            buildTree(rootNode, 0, 0.0f);
            
            MockOpenGL::resetCounters();
            root.testSortChildren();
            root.render();
            
            return MockOpenGL::getDrawCallCount();
        };
    }
}