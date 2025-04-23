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
#include "Camera.h" // Add missing include
#include <GLFW/glfw3.h> // Add missing include

class Examples {
public:
    // Update constructor declaration
    Examples(Camera* cam, GLFWwindow* win);
    ~Examples() = default;
    
    // Initialize the examples
    void initialize();
    
    // Render the examples
    void render();
    
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