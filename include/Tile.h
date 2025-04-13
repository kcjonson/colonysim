#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Rendering/Shapes/Shape.h"
#include "Rendering/Layer.h"

namespace Rendering {

class Tile : public Layer {
public:
    // Constructor with initialization parameters
    Tile(const glm::vec2& position = glm::vec2(0.0f),
         float height = 0.0f,
         float resource = 0.0f,
         int type = 0,
         const glm::vec4& color = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f),
         bool visible = true);
    ~Tile() override = default;

    // Getters and setters
    float getHeight() const { return height; }
    void setHeight(float h) { height = h; }

    float getResource() const { return resource; }
    void setResource(float r) { resource = r; }

    int getType() const { return type; }
    void setType(int t) { type = t; }

    const glm::vec4& getColor() const { return color; }
    void setColor(const glm::vec4& c) { color = c; }

    // Layer method implementations
    void setVisible(bool visible);

    // Virtual method override
    void render(bool batched = false) override;

    // Begin/end batch needs to be propagated to ensure proper batching
    void beginBatch() override;
    void endBatch() override;

    // Update all shapes' position relative to tile position
    void updatePosition(const glm::vec2& tilePosition);

    // Initialize the tile with a default shape
    void initializeDefaultShape();

protected:
    float height = 0.0f;
    float resource = 0.0f;
    int type = 0; // 0 = empty, 1 = terrain, etc.
    glm::vec4 color = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f); // Default green
};

} // namespace Rendering 