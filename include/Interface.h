#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <string>
#include "VectorGraphics.h"
#include "FontRenderer.h"
#include "Rendering/Layer.h"
#include "Rendering/Shapes/Rectangle.h"
#include "Rendering/Shapes/Text.h"
#include <GLFW/glfw3.h>

class VectorGraphics;
struct GLFWwindow;

class Interface {
public:
    Interface();
    ~Interface() = default;

    // Initialize font renderer
    bool initialize();
    
    // Initialize with graphics and window reference
    bool initializeGraphics(GLFWwindow* window);
    
    void update(float deltaTime);
    
    // Main render method for UI elements
    void render(VectorGraphics& graphics, const glm::mat4& projectionMatrix);

    // Getter for FontRenderer
    FontRenderer& getFontRenderer() { return fontRenderer; }

    // Setters for display information
    void setCursorWorldPosition(const glm::vec2& position) { 
        cursorWorldPosition = position; 
        updateCursorText();
    }
    
    void setFPS(float fps) { 
        currentFPS = fps; 
        updateFPSText();
    }

private:
    // Initialize UI components
    void initializeUIComponents();
    
    // Update UI component texts
    void updateCursorText();
    void updateFPSText();
    
    // UI properties
    const float UI_FONT_SCALE = 0.3f;
    const float UI_PADDING = 10.0f;
    const float UI_LINE_HEIGHT = 20.0f;
    const float INFO_PANEL_X = 90.0f;
    const float INFO_PANEL_Y = 90.0f;
    const float INFO_PANEL_WIDTH = 200.0f;
    const float INFO_PANEL_HEIGHT = 60.0f;
    
    // Member variables
    glm::vec2 cursorWorldPosition;
    float currentFPS;
    FontRenderer fontRenderer;
    
    // For organizing UI rendering
    std::shared_ptr<Rendering::Layer> uiLayer;
    GLFWwindow* targetWindow;
    
    // UI components (stored to avoid recreation on each render)
    std::shared_ptr<Rendering::Shapes::Rectangle> infoPanelBackground;
    std::shared_ptr<Rendering::Shapes::Text> cursorPositionText;
    std::shared_ptr<Rendering::Shapes::Text> fpsText;
}; 