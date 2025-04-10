#pragma once

#include "Rendering/Shapes/Shape.h"
#include "Rendering/Styles/Shape.h"
#include <glm/glm.hpp>

namespace Rendering {
namespace Shapes {

class Circle : public Shape {
public:
    Circle(
        const glm::vec2& position = glm::vec2(0.0f),
        float radius = 1.0f,
        const Styles::Circle& style = Styles::Circle({}),
        float zIndex = 0.0f
    );
    virtual ~Circle() = default;

    // Getters and setters
    float getRadius() const { return radius; }
    void setRadius(float r) { radius = r; markDirty(); }

    const Styles::Circle& getStyle() const { return style; }
    void setStyle(const Styles::Circle& s) { style = s; markDirty(); }

    // Implementation of the draw method
    virtual void draw(VectorGraphics& graphics) override;

private:
    float radius;
    Styles::Circle style;
};

} // namespace Shapes
} // namespace Rendering 