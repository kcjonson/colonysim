#include "Tile.h"
#include "Rendering/Shapes/Shape.h"
#include "Rendering/Shapes/Rectangle.h"
#include "VectorGraphics.h"
#include "World.h"
#include <iostream>

namespace Rendering {

// Match the tile size used in World.h (20.0f)
constexpr float TILE_SIZE = 20.0f;

Tile::Tile(const glm::vec2& position, float height, float resource, int type, const glm::vec4& color, bool visible)
    : Layer(0.0f)
    , height(height)
    , resource(resource)
    , type(type)
    , color(color) {
    setVisible(visible);
    initializeDefaultShape();
    updatePosition(position);
}

void Tile::initializeDefaultShape() {
    // Create a default rectangle shape for the tile
    auto rect = std::make_shared<Shapes::Rectangle>(
        glm::vec2(-TILE_SIZE/2.0f, -TILE_SIZE/2.0f),
        glm::vec2(TILE_SIZE),
        Styles::Rectangle({
            .color = color
        })
    );
    addItem(rect);
}

void Tile::setVisible(bool visible) {
    Layer::setVisible(visible);
}

void Tile::updatePosition(const glm::vec2& tilePosition) {
    for (const auto& child : children) {
        if (auto shape = std::dynamic_pointer_cast<Shapes::Shape>(child)) {
            // Set the shape's position directly to the tile position
            shape->setPosition(tilePosition);
        }
    }
}

void Tile::render(VectorGraphics& graphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (!visible) {
        return;
    }

    // Render all shapes
    for (const auto& child : children) {
        if (auto shape = std::dynamic_pointer_cast<Shapes::Shape>(child)) {
            shape->render(graphics, viewMatrix, projectionMatrix);
        }
    }
}

void Tile::renderScreenSpace(VectorGraphics& graphics, const glm::mat4& projectionMatrix) {
    if (!visible) return;
    Layer::renderScreenSpace(graphics, projectionMatrix);
}

void Tile::beginBatch(VectorGraphics& graphics) {
    Layer::beginBatch(graphics);
}

void Tile::endBatch(VectorGraphics& graphics) {
    Layer::endBatch(graphics);
}

} // namespace Rendering 