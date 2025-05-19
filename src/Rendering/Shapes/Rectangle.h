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
    // Alias for the Rectangle style for convenience
    using Styles = Rendering::Styles::Rectangle;
    
    /**
     * Arguments struct for Rectangle constructor
     */
    struct Args {
        glm::vec2 position = glm::vec2(0.0f);
        glm::vec2 size = glm::vec2(1.0f);
        Styles style = Styles({});
        float zIndex = 0.0f;
    };
      /**
     * Create a rectangle using the Args struct.
     *
     * @param args A struct containing all arguments for the rectangle.
     */
    explicit Rectangle(const Args& args);
    
    virtual ~Rectangle() = default;    // Getters and setters
    const glm::vec2& getSize() const { return size; }
    void setSize(const glm::vec2& newSize) { size = newSize; markDirty(); }

    const Styles& getStyle() const { return style; }
    void setStyle(const Styles& s) { style = s; markDirty(); }

    // Implementation of the draw method
    virtual void draw() override;

private:
    glm::vec2 size;
    Styles style;
};

} // namespace Shapes
} // namespace Rendering