#pragma once

#include "Rendering/Shapes/Shape.h"
#include "Rendering/Styles/Shape.h"
#include <glm/glm.hpp>

namespace Rendering {
namespace Shapes {

class Line : public Shape {
public:
    Line(const glm::vec2& start = glm::vec2(0.0f),
         const glm::vec2& end = glm::vec2(1.0f),
         const Styles::Line& style = Styles::Line(),
         float zIndex = 0.0f);
    virtual ~Line() = default;

    // Getters and setters
    const glm::vec2& getStart() const { return start; }
    void setStart(const glm::vec2& s) { start = s; markDirty(); }

    const glm::vec2& getEnd() const { return end; }
    void setEnd(const glm::vec2& e) { end = e; markDirty(); }

    const Styles::Line& getStyle() const { return style; }
    void setStyle(const Styles::Line& s) { style = s; markDirty(); }

    // Implementation of the draw method
    virtual void draw(VectorGraphics& graphics) override;

private:
    glm::vec2 start;
    glm::vec2 end;
    Styles::Line style;
};

} // namespace Shapes
} // namespace Rendering 