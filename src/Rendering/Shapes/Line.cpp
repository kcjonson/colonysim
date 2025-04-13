#include "Rendering/Shapes/Line.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Line::Line(const glm::vec2& start, const glm::vec2& end, const Styles::Line& style, float zIndex)
    : Shape(start, style, zIndex)
    , start(start)
    , end(end)
    , style(style) {
}

void Line::draw() {
    // Use the width property from the style
    VectorGraphics::getInstance().drawLine(start, end, style.color, style.width);
}

} // namespace Shapes
} // namespace Rendering 