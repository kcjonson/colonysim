#include "Rendering/Shapes/Circle.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Circle::Circle(const glm::vec2& position, float radius, const glm::vec4& color, float zIndex, int segments)
    : Shape(position, color, zIndex)
    , radius(radius)
    , segments(segments) {
}

void Circle::draw(VectorGraphics& graphics) {
    graphics.drawCircle(position, radius, color, segments);
}

} // namespace Shapes
} // namespace Rendering 