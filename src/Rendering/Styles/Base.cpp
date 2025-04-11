#include "Rendering/Styles/Base.h"

namespace Rendering {
namespace Styles {

Base::Base(const glm::vec4& color, float opacity)
    : color(color)
    , opacity(opacity) {
    // Only apply opacity to the alpha channel, preserving the original color values
    this->color.a = color.a * opacity;
}

} // namespace Styles
} // namespace Rendering 