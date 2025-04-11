#include "Rendering/Shapes/Rectangle.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Rectangle::Rectangle(const glm::vec2& position, const glm::vec2& size, const Styles::Rectangle& style, float zIndex)
    : Shape(position, style, zIndex)
    , size(size)
    , style(style) {
}

void Rectangle::draw(VectorGraphics& graphics) {
    graphics.drawRectangle(
        position,
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