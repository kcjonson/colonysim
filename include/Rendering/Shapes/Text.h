#pragma once

#include "Rendering/Shapes/Shape.h"
#include <glm/glm.hpp>
#include <string>

namespace Rendering {
namespace Shapes {

class Text : public Shape {
public:
    Text(
        const std::string& text = "",
        const glm::vec2& position = glm::vec2(0.0f),
        const glm::vec4& color = glm::vec4(1.0f),
        float zIndex = 0.0f
    );
    virtual ~Text() = default;

    // Getters and setters
    const std::string& getText() const { return text; }
    void setText(const std::string& t) { text = t; markDirty(); }

    // Implementation of the draw method
    virtual void draw(VectorGraphics& graphics) override;

private:
    std::string text;
};

} // namespace Shapes
} // namespace Rendering 