#pragma once

#include <memory>
#include "Rendering/Layer.h"
#include "Rendering/Shapes/Rectangle.h"
#include "Rendering/Shapes/Circle.h"
#include "Rendering/Shapes/Line.h"
#include "Rendering/Shapes/Polygon.h"
#include "Rendering/Shapes/Text.h"
#include "VectorGraphics.h"
#include <glm/glm.hpp>

class Examples {
public:
    Examples();
    ~Examples() = default;
    
    // Initialize the examples
    void initialize();
    
    // Render the examples
    void render(VectorGraphics& vectorGraphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    
private:
    // Create examples for different shapes
    void createRectangleExamples();
    void createCircleExamples();
    void createLineExamples();
    void createPolygonExamples();
    void createTextExamples();
    
    // Layer to hold all the example shapes
    std::shared_ptr<Rendering::Layer> examplesLayer;
}; 