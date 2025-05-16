#include "Rendering/Shapes/Text.h"
#include "VectorGraphics.h"
#include "FontRenderer.h" 
#include "Renderer.h"

namespace Rendering {
namespace Shapes {

/*
TODO: implement a width and height for the text, as well as an attribute called "overflow"
- height should be able to be set both in hard units and number of lines
- overflow should have four behaviors that can be set, crop (default), wrap, show, and ellipsis
- wrap should wrap the text to the width
- overflow should overflow the text to as wide as it need to be
- crop should crop the text to the width at the character level (not pixel)
- ellipsis truncate should truncate the text to the width (minus the space for the ellipsis) and add an ellipsis at the end
*/

Text::Styles::Styles(const StyleParams& params)
    : Rendering::Styles::Base(params.color, params.opacity)
    , fontSize(params.fontSize)
    , horizontalAlign(params.horizontalAlign)
    , verticalAlign(params.verticalAlign) 
    , size(params.size) {
}

Text::Text(const Args& args)
    : Shape(args.position, args.style, args.zIndex)
    , text(args.text)
    , style(args.style) {
}



void Text::draw() {
    // Get reference to VectorGraphics
    VectorGraphics& vectorGraphics = VectorGraphics::getInstance();
    
    // Measure the text dimensions using VectorGraphics
    // The scale factor may need adjustment based on your specific implementation
    glm::vec2 textSize = vectorGraphics.measureText(text, style.fontSize / 48.0f);
    
    // Determine box size (use measured text size if size is zero)
    glm::vec2 boxSize = style.size.x <= 0 || style.size.y <= 0 ? textSize : style.size;
    
    // Calculate the position adjustment based on alignment
    glm::vec2 alignedPosition = position;
    
    // Apply horizontal alignment
    switch (style.horizontalAlign) {
        case TextAlign::Horizontal::Left:
            // No adjustment needed for left alignment
            break;
        case TextAlign::Horizontal::Center:
            alignedPosition.x += (boxSize.x - textSize.x) / 2.0f;
            break;
        case TextAlign::Horizontal::Right:
            alignedPosition.x += boxSize.x - textSize.x;
            break;
    }
    
    // Apply vertical alignment
    switch (style.verticalAlign) {
        case TextAlign::Vertical::Top:
            // No adjustment needed for top alignment if the font renderer draws from the top
            // If the font renderer uses baseline positioning, you may need to adjust
            break;
        case TextAlign::Vertical::Middle:
            alignedPosition.y += (boxSize.y - textSize.y) / 2.0f;
            break;
        case TextAlign::Vertical::Bottom:
            alignedPosition.y += boxSize.y - textSize.y;
            break;
    }
    
    // Draw the text at the aligned position using VectorGraphics
    // Convert style.color to an appropriate format (vec4 for VectorGraphics)
    glm::vec4 color(style.color.r, style.color.g, style.color.b, style.color.a);
    vectorGraphics.drawText(text, alignedPosition, color);
}

} // namespace Shapes
} // namespace Rendering