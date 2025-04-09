#pragma once

#include "Rendering/Shapes/Shape.h"
#include <glm/glm.hpp>
#include <vector>

namespace Rendering {
namespace Shapes {

class Polygon : public Shape {
public:
    Polygon(
        const std::vector<glm::vec2>& points = {},
        const glm::vec4& color = glm::vec4(1.0f),
        float zIndex = 0.0f
    );
    virtual ~Polygon() = default;

    // Getters and setters
    const std::vector<glm::vec2>& getPoints() const { return points; }
    void setPoints(const std::vector<glm::vec2>& p) { points = p; markDirty(); }
    
    void addPoint(const glm::vec2& point) { points.push_back(point); markDirty(); }
    void clearPoints() { points.clear(); markDirty(); }

    // Implementation of the draw method
    virtual void draw(VectorGraphics& graphics) override;

private:
    std::vector<glm::vec2> points;
};

} // namespace Shapes
} // namespace Rendering 