#pragma once

#include "Rendering/Shapes/Shape.h"
#include "Rendering/Styles/Shape.h"
#include <glm/glm.hpp>

namespace Rendering {
namespace Shapes {

/**
 * A rectangle shape drawn from its top-left corner
 * 
 * The position of the rectangle is its top-left corner, not its center.
 * This aligns with common 2D drawing APIs and the Text class.
 */
class Rectangle : public Shape {
public:
    /**
     * Create a rectangle with the specified position (top-left corner), size, style and z-index
     * 
     * @param position The top-left corner position of the rectangle
     * @param size The width and height of the rectangle
     * @param style The style properties of the rectangle
     * @param zIndex The z-index for depth ordering
     */
    Rectangle(
        const glm::vec2& position = glm::vec2(0.0f),
        const glm::vec2& size = glm::vec2(1.0f),
        const Styles::Rectangle& style = Styles::Rectangle({}),
        float zIndex = 0.0f
    );
    
    virtual ~Rectangle() = default;

    // Getters and setters
    const glm::vec2& getSize() const { return size; }
    void setSize(const glm::vec2& newSize) { size = newSize; markDirty(); }

    const Styles::Rectangle& getStyle() const { return style; }
    void setStyle(const Styles::Rectangle& s) { style = s; markDirty(); }

    // Implementation of the draw method
    virtual void draw() override;

private:
    glm::vec2 size;
    Styles::Rectangle style;
};

} // namespace Shapes
} // namespace Rendering