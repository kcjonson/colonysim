#include "Rendering/Shapes/Rectangle.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Rectangle::Rectangle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float zIndex)
    : Shape(position, color, zIndex)
    , size(size) {
}

void Rectangle::draw(VectorGraphics& graphics) {
    graphics.drawRectangle(position, size, color);
}

} // namespace Shapes
} // namespace Rendering 