#include "Rendering/Shapes/Text.h"
#include "FontRenderer.h"
#include "Renderer.h"
#include "VectorGraphics.h"
#include <iostream>

namespace Rendering {
	namespace Shapes {

		/*
		TODO: implement a width and height for the text, as well as an attribute called "overflow"
		- height should be able to be set both in hard units and number of lines
		- overflow should have four behaviors that can be set, crop (default), wrap, show, and ellipsis
		- wrap should wrap the text to the width
		- overflow should overflow the text to as wide as it need to be
		- crop should crop the text to the width at the character level (not pixel)
		- ellipsis truncate should truncate the text to the width (minus the space for the ellipsis) and add an ellipsis
		at the end
		*/

		Text::Text(const Args &args)
			: Shape() {
			style = args.style;
			text = args.text;
			position = args.position;
			size = args.size;
			zIndex = args.zIndex;
		}

		void Text::draw() {
			if (!text.empty()) {
				VectorGraphics &vectorGraphics = VectorGraphics::getInstance();

				// Determine measured width and consistent line height
				glm::vec2 textSize = vectorGraphics.measureText(text, style.fontSize);
				float lineHeight = Renderer::getInstance().getLineHeight(style.fontSize);

				// Determine box size (use measured text size if size is zero)
				glm::vec2 boxSize = size.x <= 0 || size.y <= 0 ? textSize : size;

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
						alignedPosition.y += (boxSize.y - lineHeight) / 2.0f;
						break;
					case TextAlign::Vertical::Bottom:
						alignedPosition.y += boxSize.y - lineHeight;
						break;
				}

				// Draw the text at the aligned position using VectorGraphics
				// Apply both color and opacity when drawing
				vectorGraphics.drawText(text, alignedPosition, style.color, style.fontSize);
			}
		}

		float Text::measureTextWidth(const std::string &text) const {
			VectorGraphics &vectorGraphics = VectorGraphics::getInstance();
			return vectorGraphics.measureText(text, style.fontSize).x;
		}
	} // namespace Shapes
} // namespace Rendering