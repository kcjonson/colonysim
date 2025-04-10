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

    // Draw this shape
    draw(graphics);
    
    // NOTE: Shapes shouldn't have children, so don't call Layer::render
}

void Shape::renderScreenSpace(VectorGraphics& graphics, const glm::mat4& projectionMatrix) {
    if (!visible) return;

    // Draw this shape
    draw(graphics);
    
    // NOTE: Shapes shouldn't have children, so don't call Layer::renderScreenSpace
    // We remove this call to avoid recursive rendering that might be causing performance issues
}

} // namespace Shapes
} // namespace Rendering 