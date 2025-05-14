#include "Tile.h"
#include "../../Rendering/Shapes/Rectangle.h"
#include "../../Rendering/Shapes/Circle.h"
#include <glm/glm.hpp>
#include "World.h"
#include <iostream>

namespace Rendering {

// Match the tile size used in World.h (20.0f)
constexpr float TILE_SIZE = 20.0f;

// Default colors for different terrain types
const glm::vec4 WATER_COLOR = glm::vec4(0.0f, 0.0f, 0.8f, 1.0f);  // Dark blue for water
const glm::vec4 GRASS_COLOR = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f);  // Green for grass
const glm::vec4 MOUNTAIN_COLOR = glm::vec4(0.5f, 0.35f, 0.05f, 1.0f); // Brown for mountains

Tile::Tile(const glm::vec2& position, float height, float resource, WorldGen::TerrainType type, const glm::vec4& color, bool visible)
    : Layer(0.1f) // Default z-index for tiles
    , height(height)
    , resource(resource)
    , type(type)
    , color(color) {
    setVisible(visible);
    initializeDefaultShape();
    updatePosition(position);
}

void Tile::initializeDefaultShape() {
    // Create a 20x20 rectangle with the tile's color (top-left positioned)
    auto rect = std::make_shared<Shapes::Rectangle>(
        glm::vec2(0, 0),  // Position will be set by updatePosition
        glm::vec2(20, 20), // Default tile size
        Styles::Rectangle({
            .color = color, // Use the tile's color
        })
    );
    
    // Add it to the tile's children
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

void Tile::render(bool batched) {
    if (!visible) {
        return;
    }

    // Render all children using the base implementation
    Layer::render(batched);
}

void Tile::beginBatch() {
    Layer::beginBatch();
}

void Tile::endBatch() {
    Layer::endBatch();
}

} // namespace Rendering