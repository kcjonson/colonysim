#include "Rendering/Styles/Shape.h"

namespace Rendering {
namespace Styles {

Rectangle::Rectangle(const glm::vec4& color,
                    float opacity,
                    const glm::vec4& borderColor,
                    float borderWidth,
                    BorderPosition borderPosition,
                    float cornerRadius)
    : Base(color, opacity)
    , Border(borderColor, borderWidth, borderPosition, cornerRadius) {
}

Circle::Circle(const glm::vec4& color,
              float opacity,
              const glm::vec4& borderColor,
              float borderWidth,
              BorderPosition borderPosition)
    : Base(color, opacity)
    , Border(borderColor, borderWidth, borderPosition) {
}

Line::Line(const glm::vec4& color, float opacity)
    : Base(color, opacity) {
}

Text::Text(const glm::vec4& color, float opacity)
    : Base(color, opacity) {
}

Polygon::Polygon(const glm::vec4& color,
                float opacity,
                const glm::vec4& borderColor,
                float borderWidth,
                BorderPosition borderPosition)
    : Base(color, opacity)
    , Border(borderColor, borderWidth, borderPosition) {
}

} // namespace Styles
} // namespace Rendering 