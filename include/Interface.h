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
#include "GameState.h"

class VectorGraphics;
struct GLFWwindow;

class Interface {
public:
    Interface(GameState& gameState);
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

private:
    // Static list of GameState properties to display
    static const std::vector<std::string> GAME_STATE_PROPERTIES;
    // Initialize UI components
    void initializeUIComponents();
    
    // UI properties
    const float UI_FONT_SCALE = 0.3f;
    const float UI_PADDING = 10.0f;
    const float UI_LINE_HEIGHT = 20.0f;
    const float INFO_PANEL_X = 90.0f;
    const float INFO_PANEL_Y = 90.0f;
    const float INFO_PANEL_WIDTH = 200.0f;
    
    // Member variables
    FontRenderer fontRenderer;
    
    // For organizing UI rendering
    std::shared_ptr<Rendering::Layer> uiLayer;
    GLFWwindow* targetWindow;
    
    // UI components
    std::shared_ptr<Rendering::Shapes::Rectangle> infoPanelBackground;
    std::vector<std::shared_ptr<Rendering::Shapes::Text>> propertyTexts;

    GameState& gameState;
}; 