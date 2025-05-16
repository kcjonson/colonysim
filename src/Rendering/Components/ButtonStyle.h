#pragma once

#include "Rendering/Styles/Shape.h"
#include <glm/glm.hpp>

namespace Rendering {
namespace Styles {

// Parameter struct for button styling
struct ButtonStyleParams {
    glm::vec4 color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    float opacity = 1.0f;
    glm::vec4 borderColor = glm::vec4(0.0f);
    float borderWidth = 0.0f;
    BorderPosition borderPosition = BorderPosition::Outside;
    float cornerRadius = 5.0f;
    glm::vec4 hoverColor = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
};

// Button style class
class Button : public Rectangle {
public:
    Button(const ButtonStyleParams& params = {}) 
        : Rectangle(RectangleStyleParams{
            .color = params.color,
            .opacity = params.opacity,
            .borderColor = params.borderColor,
            .borderWidth = params.borderWidth,
            .borderPosition = params.borderPosition,
            .cornerRadius = params.cornerRadius
          }),
          color(params.color),
          hoverColor(params.hoverColor) {
    }

    glm::vec4 color;
    glm::vec4 hoverColor;
};

} // namespace Styles
} // namespace Rendering
