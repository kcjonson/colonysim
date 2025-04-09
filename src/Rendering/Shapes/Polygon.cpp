#include "Rendering/Shapes/Polygon.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Polygon::Polygon(const std::vector<glm::vec2>& points, const glm::vec4& color, float zIndex)
    : Shape(glm::vec2(0.0f), color, zIndex) // Position is not used directly for polygons
    , points(points) {
}

void Polygon::draw(VectorGraphics& graphics) {
    // Only draw if we have enough points
    if (points.size() >= 3) {
        graphics.drawPolygon(points, color);
    }
}

} // namespace Shapes
} // namespace Rendering 