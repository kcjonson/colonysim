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
    Layer(float zIndex = 0.0f, ProjectionType projType = ProjectionType::WorldSpace);
    virtual ~Layer() = default;

    // Hierarchy methods - renamed for more consistent naming
    void addItem(std::shared_ptr<Layer> item);
    void removeItem(std::shared_ptr<Layer> item);
    void clearItems();
    
    // Z-index handling
    float getZIndex() const { return zIndex; }
    void setZIndex(float z) { zIndex = z; }

    // Visibility
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

    // Projection type getter/setter
    ProjectionType getProjectionType() const { return projectionType; }
    void setProjectionType(ProjectionType type) { projectionType = type; }

    // Access to children
    const std::vector<std::shared_ptr<Layer>>& getChildren() const { return children; }

    // Setup camera and window for projection calculations
    void setCamera(Camera* cam) { camera = cam; }
    void setWindow(GLFWwindow* win) { window = win; }
    
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
    

protected:
    float zIndex;
    bool visible;
    ProjectionType projectionType;
    std::vector<std::shared_ptr<Layer>> children;
    Camera* camera;
    GLFWwindow* window;

    // Sort children by z-index before rendering
    void sortChildren();
};

} // namespace Rendering 