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
    
    // Draw the polygon with both fill and border
    graphics.drawPolygon(
        offsetVertices,
        style.color,                // Use base color
        style.borderColor,          // Use border color
        style.borderWidth,          // Use border width
        static_cast<BorderPosition>(style.borderPosition)  // Convert to VectorGraphics border position
    );
}

} // namespace Shapes
} // namespace Rendering 