#pragma once

#include "Rendering/Shapes/Shape.h"
#include <glm/glm.hpp>

namespace Rendering {
namespace Shapes {

class Circle : public Shape {
public:
    Circle(
        const glm::vec2& position = glm::vec2(0.0f),
        float radius = 1.0f,
        const glm::vec4& color = glm::vec4(1.0f),
        float zIndex = 0.0f,
        int segments = 32
    );
    virtual ~Circle() = default;

    // Getters and setters
    float getRadius() const { return radius; }
    void setRadius(float r) { radius = r; markDirty(); }

    int getSegments() const { return segments; }
    void setSegments(int s) { segments = s; markDirty(); }

    // Implementation of the draw method
    virtual void draw(VectorGraphics& graphics) override;

private:
    float radius;
    int segments;
};

} // namespace Shapes
} // namespace Rendering 