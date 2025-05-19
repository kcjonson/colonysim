#include "Rendering/Shapes/Rectangle.h"
#include "VectorGraphics.h"
#include <iostream>

namespace Rendering {
namespace Shapes {

Rectangle::Rectangle(const glm::vec2& position, const glm::vec2& size, const Styles& style, float zIndex)
    : Shape(position, style, zIndex)
    , size(size)
    , style(style) {
}

Rectangle::Rectangle(const Args& args)
    : Shape(args.position, args.style, args.zIndex)
    , size(args.size)
    , style(args.style) {
}

void Rectangle::draw() {
    // Convert top-left position to center position for VectorGraphics::drawRectangle
    glm::vec2 centerPosition = position + size / 2.0f;
    
    // Draw the rectangle with its center position
    VectorGraphics::getInstance().drawRectangle(
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