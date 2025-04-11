#pragma once
#include <glm/glm.hpp>

namespace Rendering {
namespace Styles {

class Base {
public:
    Base(const glm::vec4& color = glm::vec4(1.0f), float opacity = 1.0f);
    virtual ~Base() = default;

    glm::vec4 color;
    float opacity; // this will change the opacity of both the fill and border
};

} // namespace Styles
} // namespace Rendering 