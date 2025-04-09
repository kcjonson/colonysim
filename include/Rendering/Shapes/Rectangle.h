#pragma once

#include "Rendering/Shapes/Shape.h"
#include <glm/glm.hpp>

namespace Rendering {
namespace Shapes {

class Rectangle : public Shape {
public:
    Rectangle(
        const glm::vec2& position = glm::vec2(0.0f),
        const glm::vec2& size = glm::vec2(1.0f),
        const glm::vec4& color = glm::vec4(1.0f),
        float zIndex = 0.0f
    );
    virtual ~Rectangle() = default;

    // Getters and setters
    const glm::vec2& getSize() const { return size; }
    void setSize(const glm::vec2& sz) { size = sz; markDirty(); }

    // Implementation of the draw method
    virtual void draw(VectorGraphics& graphics) override;

private:
    glm::vec2 size;
};

} // namespace Shapes
} // namespace Rendering 