#include "Rendering/Shapes/Rectangle.h"
#include "VectorGraphics.h"
#include <iostream>

namespace Rendering {
namespace Shapes {

Rectangle::Rectangle(const glm::vec2& position, const glm::vec2& size, const Styles::Rectangle& style, float zIndex)
    : Shape(position, style, zIndex)
    , size(size)
    , style(style) {
}

void Rectangle::draw(VectorGraphics& graphics) {
    // Convert top-left position to center position for VectorGraphics::drawRectangle
    glm::vec2 centerPosition = position + size / 2.0f;
    
    // Draw the rectangle with its center position
    graphics.drawRectangle(
        centerPosition,
        size,
        style.color,                // Use base color
        style.borderColor,          // Use border color
        style.borderWidth,          // Use border width
        static_cast<BorderPosition>(style.borderPosition), // Convert to VectorGraphics border position
        style.cornerRadius          // Use corner radius
    );
}

} // namespace Shapes
} // namespace Rendering 