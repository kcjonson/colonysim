#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>

class VectorGraphics;
struct GLFWwindow;
class Camera;
class Renderer;

// Forward declaration for Shape
namespace Rendering {
    namespace Shapes {
        class Shape;
    }
}

namespace Rendering {

// Define coordinate systems for different layer types
enum class ProjectionType {
    WorldSpace,    // Uses the world coordinate system with camera transformation
    ScreenSpace    // Uses screen coordinates (0,0 at top-left, width,height at bottom-right)
};

class Layer {
public:
    // Updated constructor to accept optional camera and window
    Layer(float zIndex = 0.0f, 
          ProjectionType projType = ProjectionType::WorldSpace,
          Camera* cam = nullptr, 
          GLFWwindow* win = nullptr);
    virtual ~Layer() = default;

    // Hierarchy methods - renamed for more consistent naming
    void addItem(std::shared_ptr<Layer> item);
    void removeItem(std::shared_ptr<Layer> item);
    void clearItems();
    
    // Z-index handling
    float getZIndex() const { return zIndex; }
    void setZIndex(float z); // Modified to handle parent notification

    // Visibility
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

    // Projection type getter/setter
    ProjectionType getProjectionType() const { return projectionType; }
    void setProjectionType(ProjectionType type) { projectionType = type; }

    // Access to children
    const std::vector<std::shared_ptr<Layer>>& getChildren() const { return children; }

    // REMOVED setCamera and setWindow methods
    // void setCamera(Camera* cam); // REMOVED
    // void setWindow(GLFWwindow* win); // REMOVED
    
    // Get camera and window
    Camera* getCamera() const { return camera; }
    GLFWwindow* getWindow() const { return window; }
    
    // Get matrices based on projection type
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    // Rendering method - handles all projection types
    // The batched parameter indicates whether this is part of a batch operation
    virtual void render(bool batched = false);

    // Begin/end batch needs to be propagated to ensure proper batching
    virtual void beginBatch();
    virtual void endBatch();
    
    // For testing purposes - public access to sortChildren
    void testSortChildren() { sortChildren(); }

    // For testing purposes - access to sortChildren
    #ifdef TESTING
    void exposeSortChildren() { sortChildren(); }
    #endif

    // Friend class for testing
    friend class MockLayer;

protected:
    float zIndex;
    bool visible;
    ProjectionType projectionType;
    std::vector<std::shared_ptr<Layer>> children;
    Camera* camera;
    GLFWwindow* window;
    Layer* parent = nullptr; // Added parent pointer
    bool childrenNeedSorting = false; // Added dirty flag

    // Sort children by z-index before rendering (conditionally)
    void sortChildren();

    // Helper to set parent (used internally)
    void setParent(Layer* p);
};

} // namespace Rendering