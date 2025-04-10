#include "Rendering/Styles/Base.h"

namespace Rendering {
namespace Styles {

Base::Base(const glm::vec4& color, float opacity)
    : color(color)
    , opacity(opacity) {
}

} // namespace Styles
} // namespace Rendering 