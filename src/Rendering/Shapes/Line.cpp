#include "Rendering/Shapes/Line.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Line::Line(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float width, float zIndex)
    : Shape(glm::vec2(0.0f), color, zIndex) // Position is not used directly in line shapes
    , start(start)
    , end(end)
    , width(width) {
}

void Line::draw(VectorGraphics& graphics) {
    graphics.drawLine(start, end, color, width);
}

} // namespace Shapes
} // namespace Rendering 