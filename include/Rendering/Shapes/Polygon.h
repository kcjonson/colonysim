#pragma once

#include "Rendering/Shapes/Shape.h"
#include "Rendering/Styles/Shape.h"
#include <glm/glm.hpp>
#include <vector>

namespace Rendering {
namespace Shapes {

class Polygon : public Shape {
public:
    Polygon(const glm::vec2& position = glm::vec2(0.0f),
            const std::vector<glm::vec2>& vertices = {},
            const Styles::Polygon& style = Styles::Polygon({}),
            float zIndex = 0.0f);
    virtual ~Polygon() = default;

    // Getters and setters
    const std::vector<glm::vec2>& getVertices() const { return vertices; }
    void setVertices(const std::vector<glm::vec2>& v) { vertices = v; markDirty(); }
    
    const Styles::Polygon& getStyle() const { return style; }
    void setStyle(const Styles::Polygon& s) { style = s; markDirty(); }

    // Implementation of the draw method
    virtual void draw() override;

private:
    std::vector<glm::vec2> vertices;
    Styles::Polygon style;
};

} // namespace Shapes
} // namespace Rendering 