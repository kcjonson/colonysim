#include "Rendering/Shapes/Text.h"
#include "VectorGraphics.h"

namespace Rendering {
namespace Shapes {


    /*
    TODO: implement a width and height for the text
    - width should have four behaviors that can be set, crop (default), wrap, fit, and ellipsis truncate
    - height should be able to be set both in hard units and number of lines
    - wrap should wrap the text to the width
    - fit should shink the text to the width
    - crop should crop the text to the width at the character level (not pixel)
    - ellipsis truncate should truncate the text to the width
    - if not set, it should have its current behavior
    
    */

Text::Text(const std::string& text, const glm::vec2& position, const Styles::Text& style, float zIndex)
    : Shape(position, style, zIndex)
    , text(text)
    , style(style) {
}

void Text::draw(VectorGraphics& graphics) {
    // In the future, could add support for text shadows, outlines, etc.
    graphics.drawText(text, position, style.color);
}

} // namespace Shapes
} // namespace Rendering 