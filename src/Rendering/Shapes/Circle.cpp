#include "Rendering/Shapes/Circle.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Circle::Circle(const glm::vec2& position, float radius, const Styles::Circle& style, float zIndex)
    : Shape(position, style, zIndex)
    , radius(radius)
    , style(style) {
}

void Circle::draw(VectorGraphics& graphics) {
    // Draw the circle with both fill and border in a single draw call
    // Note: VectorGraphics API for drawCircle needs to be updated to match this
    graphics.drawCircle(
        position,
        radius,
        style.color,                // Use base color
        style.borderColor,          // Use border color
        style.borderWidth,          // Use border width
        static_cast<BorderPosition>(style.borderPosition)  // Convert to VectorGraphics border position
    );
}

} // namespace Shapes
} // namespace Rendering 