#include "Rendering/Shapes/Rectangle.h"
#include "VectorGraphics.h"
#include <iostream>

namespace Rendering {
namespace Shapes {

Rectangle::Rectangle(const Args& args)
    : Shape(args.position, args.style, args.zIndex)
    , size(args.size)
    , style(args.style) {
}

void Rectangle::draw() {
    static int drawCallCount = 0;
    drawCallCount++;
    if (drawCallCount < 5) {
        std::cout << "Rectangle::draw() call " << drawCallCount << " at position (" << position.x << "," << position.y << ") size (" << size.x << "," << size.y << ")" << std::endl;
    }
    
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