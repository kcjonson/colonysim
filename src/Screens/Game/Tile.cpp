#include "Tile.h"
#include "../../Rendering/Shapes/Rectangle.h"
#include "../../Rendering/Shapes/Circle.h"
#include <glm/glm.hpp>
#include "World.h"
#include "../../ConfigManager.h"
#include <iostream>

namespace Rendering {

// Default colors for different terrain types
namespace {
    glm::vec4 getColorForTerrainType(WorldGen::TerrainType type) {
        switch (type) {
            case WorldGen::TerrainType::Ocean:
                return glm::vec4(0.0f, 0.2f, 0.5f, 1.0f);  // Deep blue
            case WorldGen::TerrainType::Shallow:
                return glm::vec4(0.0f, 0.5f, 0.8f, 1.0f);  // Light blue
            case WorldGen::TerrainType::Beach:
                return glm::vec4(0.9f, 0.9f, 0.6f, 1.0f);  // Sandy
            case WorldGen::TerrainType::Lowland:
                return glm::vec4(0.0f, 0.6f, 0.0f, 1.0f);  // Green
            case WorldGen::TerrainType::Highland:
                return glm::vec4(0.2f, 0.5f, 0.2f, 1.0f);  // Dark green
            case WorldGen::TerrainType::Mountain:
                return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray
            case WorldGen::TerrainType::Peak:
                return glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);  // Light gray/white
            case WorldGen::TerrainType::Volcano:
                return glm::vec4(0.6f, 0.3f, 0.3f, 1.0f);  // Reddish
            default:
                return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);  // Magenta for unknown
        }
    }
}

Tile::Tile(const glm::vec2& position, float height, float resource, WorldGen::TerrainType type, bool visible)
    : Layer(0.1f) // Default z-index for tiles
    , height(height)
    , resource(resource)
    , type(type)
    , color(getColorForTerrainType(type)) {  // Determine color based on terrain type
    setVisible(visible);
    initializeDefaultShape();
    updatePosition(position);
}

void Tile::initializeDefaultShape() {
    // Get tile size from config
    float tileSize = ConfigManager::getInstance().getTileSize();
    
    // Create a rectangle with the tile's color and a purple border
    auto rect = std::make_shared<Shapes::Rectangle>(
        Shapes::Rectangle::Args{
            .position = glm::vec2(0, 0),  // Position will be set by updatePosition
            .size = glm::vec2(tileSize, tileSize), // Use config tile size
            .style = Styles::Rectangle({
                .color = color, // Use the tile's color
                .borderColor = glm::vec4(0.5f, 0.0f, 0.5f, 1.0f), // Purple border
                .borderWidth = 1.0f, // 1 pixel border
            })
        }
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