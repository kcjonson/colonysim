#pragma once

#include "Rendering/Shapes/Shape.h"
#include "Rendering/Styles/Shape.h"
#include <glm/glm.hpp>

namespace Rendering {
namespace Shapes {

class Rectangle : public Shape {
public:
    Rectangle(
        const glm::vec2& position = glm::vec2(0.0f),
        const glm::vec2& size = glm::vec2(1.0f),
        const Styles::Rectangle& style = Styles::Rectangle({}),
        float zIndex = 0.0f
    );
    
    virtual ~Rectangle() = default;

    // Getters and setters
    const glm::vec2& getSize() const { return size; }
    void setSize(const glm::vec2& newSize) { size = newSize; markDirty(); }

    const Styles::Rectangle& getStyle() const { return style; }
    void setStyle(const Styles::Rectangle& s) { style = s; markDirty(); }

    // Implementation of the draw method
    virtual void draw(VectorGraphics& graphics) override;

private:
    glm::vec2 size;
    Styles::Rectangle style;
};

} // namespace Shapes
} // namespace Rendering 