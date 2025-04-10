#pragma once
#include <glm/glm.hpp>

namespace Rendering {
namespace Styles {

enum class BorderPosition {
    Inside,
    Outside,
    Center
};

class Border {
public:
    Border(const glm::vec4& borderColor = glm::vec4(0.0f), 
           float borderWidth = 0.0f, 
           BorderPosition borderPosition = BorderPosition::Outside,
           float cornerRadius = 0.0f);
    virtual ~Border() = default;

    glm::vec4 borderColor;
    float borderWidth;
    BorderPosition borderPosition;
    float cornerRadius;
};

} // namespace Styles
} // namespace Rendering 