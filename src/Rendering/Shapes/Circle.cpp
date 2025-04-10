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
    graphics.drawCircle(position, radius, style.color);
}

} // namespace Shapes
} // namespace Rendering 