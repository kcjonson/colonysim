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

void Shape::render() {
    if (!visible) return;
    draw();
}

} // namespace Shapes
} // namespace Rendering 