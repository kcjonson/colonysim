#pragma once

#include "Rendering/Layer.h"
#include "Rendering/Styles/Base.h"
#include <glm/glm.hpp>

class VectorGraphics;

namespace Rendering {
namespace Shapes {

class Shape : public Rendering::Layer {
public:
    Shape(const glm::vec2& position = glm::vec2(0.0f), 
          const Styles::Base& style = Styles::Base(), 
          float zIndex = 0.0f);
    virtual ~Shape() = default;

    // Getters and setters
    const glm::vec2& getPosition() const { return position; }
    void setPosition(const glm::vec2& pos) { position = pos; markDirty(); }

    const Styles::Base& getStyle() const { return style; }
    void setStyle(const Styles::Base& s) { style = s; markDirty(); }

    // Rendering methods
    virtual void render(VectorGraphics& graphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) override;
    virtual void renderScreenSpace(VectorGraphics& graphics, const glm::mat4& projectionMatrix) override;

    // Pure virtual method that each shape must implement
    virtual void draw(VectorGraphics& graphics) = 0;

protected:
    glm::vec2 position;
    Styles::Base style;
    bool dirty; // Flag to indicate if shape properties have changed

    // Mark the shape as dirty when properties change
    void markDirty() { dirty = true; }
};

} // namespace Shapes
} // namespace Rendering 