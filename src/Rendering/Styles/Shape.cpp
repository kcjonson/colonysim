#include "Rendering/Styles/Shape.h"

namespace Rendering {
namespace Styles {

Rectangle::Rectangle(const RectangleStyleParams& params)
    : Base(params.color, params.opacity)
    , Border(params.borderColor, params.borderWidth, params.borderPosition, params.cornerRadius) {
}

Circle::Circle(const CircleStyleParams& params)
    : Base(params.color, params.opacity)
    , Border(params.borderColor, params.borderWidth, params.borderPosition) {
}

Line::Line(const LineStyleParams& params)
    : Base(params.color, params.opacity)
    , width(params.width) {
}

Text::Text(const TextStyleParams& params)
    : Base(params.color, params.opacity) {
}

Polygon::Polygon(const PolygonStyleParams& params)
    : Base(params.color, params.opacity)
    , Border(params.borderColor, params.borderWidth, params.borderPosition) {
}

} // namespace Styles
} // namespace Rendering 