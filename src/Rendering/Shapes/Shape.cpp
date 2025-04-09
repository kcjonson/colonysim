#include "Rendering/Shapes/Shape.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Shape::Shape(const glm::vec2& position, const glm::vec4& color, float zIndex)
    : Layer(zIndex)
    , position(position)
    , color(color)
    , dirty(true) {
}

void Shape::render(VectorGraphics& graphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (!visible) return;

    // Draw this shape
    draw(graphics);

    // Render all children (if any)
    Layer::render(graphics, viewMatrix, projectionMatrix);
}

void Shape::renderScreenSpace(VectorGraphics& graphics, const glm::mat4& projectionMatrix) {
    if (!visible) return;

    // Draw this shape
    draw(graphics);

    // Render all children (if any)
    Layer::renderScreenSpace(graphics, projectionMatrix);
}

} // namespace Shapes
} // namespace Rendering 