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
    
    // Get matrices based on projection type
    glm::mat4 viewMatrix = getViewMatrix();
    glm::mat4 projectionMatrix = getProjectionMatrix();
    
    // Forward to renderWithMatrices
    renderWithMatrices(viewMatrix, projectionMatrix);
}

void Shape::renderWithMatrices(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (!visible) return;
    draw();
}

void Shape::renderScreenSpace(const glm::mat4& projectionMatrix) {
    if (!visible) return;
    draw();
}

} // namespace Shapes
} // namespace Rendering 