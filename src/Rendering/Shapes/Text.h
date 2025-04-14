#pragma once

#include "Rendering/Shapes/Shape.h"
#include "Rendering/Styles/Shape.h"
#include <glm/glm.hpp>
#include <string>

namespace Rendering {
namespace Shapes {

class Text : public Shape {
public:
    Text(const std::string& text = "",
         const glm::vec2& position = glm::vec2(0.0f),
         const Styles::Text& style = Styles::Text({}),
         float zIndex = 0.0f);
    virtual ~Text() = default;

    // Getters and setters
    const std::string& getText() const { return text; }
    void setText(const std::string& t) { text = t; markDirty(); }

    const Styles::Text& getStyle() const { return style; }
    void setStyle(const Styles::Text& s) { style = s; markDirty(); }

    // Implementation of the draw method
    virtual void draw() override;

private:
    std::string text;
    Styles::Text style;
};

} // namespace Shapes
} // namespace Rendering