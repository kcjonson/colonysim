#include "Rendering/Shapes/Text.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {

Text::Text(const std::string& text, const glm::vec2& position, const glm::vec4& color, float zIndex)
    : Shape(position, color, zIndex)
    , text(text) {
}

void Text::draw(VectorGraphics& graphics) {
    graphics.drawText(text, position, color);
}

} // namespace Shapes
} // namespace Rendering 