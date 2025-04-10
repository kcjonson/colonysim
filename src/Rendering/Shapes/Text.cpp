#include "Rendering/Shapes/Text.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Text::Text(const std::string& text, const glm::vec2& position, const Styles::Text& style, float zIndex)
    : Shape(position, style, zIndex)
    , text(text)
    , style(style) {
}

void Text::draw(VectorGraphics& graphics) {
    graphics.drawText(text, position, style.color);
}

} // namespace Shapes
} // namespace Rendering 