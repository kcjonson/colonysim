#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>

class VectorGraphics;

// Forward declaration for Shape
namespace Rendering {
    namespace Shapes {
        class Shape;
    }
}

namespace Rendering {

class Layer {
public:
    Layer(float zIndex = 0.0f);
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

    // Access to children
    const std::vector<std::shared_ptr<Layer>>& getChildren() const { return children; }

    // Rendering methods
    virtual void render(VectorGraphics& graphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    virtual void renderScreenSpace(VectorGraphics& graphics, const glm::mat4& projectionMatrix);

    // Begin/end batch needs to be propagated to ensure proper batching
    virtual void beginBatch(VectorGraphics& graphics);
    virtual void endBatch(VectorGraphics& graphics);

protected:
    float zIndex;
    bool visible;
    std::vector<std::shared_ptr<Layer>> children;

    // Sort children by z-index before rendering
    void sortChildren();
};

} // namespace Rendering 