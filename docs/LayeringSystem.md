# Layering System Documentation

This document describes the layering system used for rendering elements in the game with proper z-index ordering.

## Overview

The layering system provides a hierarchical approach to organizing and rendering graphical elements with depth control (z-indexing). It consists of:

1. **Rendering::Layer Class**: Base container that can hold shapes or other layers
2. **Rendering::Shapes::Shape Class**: Abstract base class for all geometric primitives
3. **Specific Shape Classes**: Implementations for rectangles, circles, lines, etc.

## Z-Index Behavior

- Every Layer and Shape has a z-index value (default: 0.0)
- Elements with lower z-index values are rendered first (appear behind elements with higher z-index)
- Z-index operates hierarchically within parent containers
- The absolute z-index of an element = parent z-index + element z-index

## Layer Hierarchy

```
VectorGraphics
├── WorldLayer (z=0)
│   ├── TerrainLayer (z=0.1)
│   │   └── Various terrain shapes
│   ├── EntityLayer (z=0.2)
│   │   └── Various entity shapes
│   └── EffectsLayer (z=0.3)
│       └── Various effect shapes
└── UILayer (z=1000)
    ├── BackgroundLayer (z=1000.1)
    │   └── Background panel shapes
    ├── ContentLayer (z=1000.2)
    │   └── Content shapes
    └── ForegroundLayer (z=1000.3)
        └── Foreground elements, tooltips, etc.
```

## Using Layers

```cpp
// Create a layer with a specific z-index
auto myLayer = std::make_shared<Rendering::Layer>(1.5f);

// Add a child layer
auto childLayer = std::make_shared<Rendering::Layer>(0.1f);  // z-index relative to parent
myLayer->addLayer(childLayer);

// Set visibility
myLayer->setVisible(true);  // or false to hide

// Change z-index
myLayer->setZIndex(2.0f);
```

## Using Shapes

Each shape type inherits from the base Shape class which itself extends Layer:

```cpp
// Create a rectangle
auto rectangle = std::make_shared<Rendering::Shapes::Rectangle>(
    glm::vec2(100.0f, 100.0f),  // position
    glm::vec2(50.0f, 30.0f),    // size
    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),  // color (red)
    0.5f  // z-index
);

// Create a circle
auto circle = std::make_shared<Rendering::Shapes::Circle>(
    glm::vec2(150.0f, 150.0f),  // position
    25.0f,  // radius
    glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),  // color (green)
    0.6f  // z-index
);

// Add shapes to a layer
myLayer->addLayer(rectangle);
myLayer->addLayer(circle);
```

## Available Shape Types

1. **Rectangle**: Draws a rectangle
2. **Circle**: Draws a circle
3. **Line**: Draws a line between two points
4. **Polygon**: Draws a polygon defined by a set of points
5. **Text**: Draws text

## Performance Considerations

- Batch rendering is used to minimize draw calls
- Layers are sorted by z-index before rendering
- The rendering engine automatically handles batching when drawing shapes

## Integrating with Existing Systems

To integrate with existing game systems:

1. Access the world and UI root layers:
   ```cpp
   auto worldLayer = game.getWorldLayer();
   auto uiLayer = game.getUILayer();
   ```

2. Add game elements to appropriate layers:
   ```cpp
   worldLayer->addLayer(entityShape);
   uiLayer->addLayer(uiElement);
   ```

## Best Practices

1. **Logical Organization**: Group related elements in their own layer
2. **Z-Index Spacing**: Leave space between z-index values for future elements
3. **Layer Hierarchy**: Use nested layers for logically grouped items
4. **Batch Similar Items**: Keep similar items in the same layer when possible
5. **Avoid Deep Nesting**: Too many nested layers can affect performance

## Project Organization

The layering system code is organized in the following structure:

- **include/Rendering/Layer.h**: Base Layer class implementation
- **include/Rendering/Shapes/Shape.h**: Base Shape class for all geometric primitives
- **include/Rendering/Shapes/**: Directory containing all shape implementations
  - Rectangle.h
  - Circle.h
  - Line.h
  - Polygon.h
  - Text.h

- **src/Rendering/Layer.cpp**: Layer class implementation
- **src/Rendering/Shapes/**: Directory containing all shape implementations
  - Shape.cpp
  - Rectangle.cpp
  - Circle.cpp
  - Line.cpp
  - Polygon.cpp
  - Text.cpp 