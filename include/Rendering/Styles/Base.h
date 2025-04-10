#pragma once
#include <glm/glm.hpp>

namespace Rendering {
namespace Styles {

class Base {
public:
    Base(const glm::vec4& color = glm::vec4(1.0f), float opacity = 1.0f);
    virtual ~Base() = default;

    glm::vec4 color;
    float opacity;
};

} // namespace Styles
} // namespace Rendering 