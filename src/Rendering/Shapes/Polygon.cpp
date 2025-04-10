#include "Rendering/Shapes/Polygon.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Polygon::Polygon(const glm::vec2& position, const std::vector<glm::vec2>& vertices, const Styles::Polygon& style, float zIndex)
    : Shape(position, style, zIndex)
    , vertices(vertices)
    , style(style) {
}

void Polygon::draw(VectorGraphics& graphics) {
    // Create a copy of vertices with position offset
    std::vector<glm::vec2> offsetVertices = vertices;
    for (auto& vertex : offsetVertices) {
        vertex += position;
    }
    graphics.drawPolygon(offsetVertices, style.color);
}

} // namespace Shapes
} // namespace Rendering 