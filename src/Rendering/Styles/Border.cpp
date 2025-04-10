#include "Rendering/Styles/Border.h"

namespace Rendering {
namespace Styles {

Border::Border(const glm::vec4& borderColor, 
               float borderWidth, 
               BorderPosition borderPosition,
               float cornerRadius)
    : borderColor(borderColor)
    , borderWidth(borderWidth)
    , borderPosition(borderPosition)
    , cornerRadius(cornerRadius) {
}

} // namespace Styles
} // namespace Rendering 