#include "Rendering/Shapes/Shape.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Shape::Shape(const glm::vec2& position, const Styles::Base& style, float zIndex)
    : Layer(zIndex)
    , position(position)
    , style(style)
    , dirty(true) {
}

void Shape::render(VectorGraphics& graphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (!visible) return;
    draw(graphics);
}

void Shape::renderScreenSpace(VectorGraphics& graphics, const glm::mat4& projectionMatrix) {
    if (!visible) return;
    draw(graphics);
}

} // namespace Shapes
} // namespace Rendering 