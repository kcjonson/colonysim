#pragma once

#include "../Screen.h"
#include <memory>
#include <glm/glm.hpp>
#include "../../Rendering/Layer.h"
#include "../../Rendering/Shapes/Rectangle.h"
#include "../../Rendering/Shapes/Text.h"
#include "../../Rendering/Components/Button.h"

class SettingsScreen : public Screen {
public:
    // Update constructor to accept Camera* and GLFWwindow*
    SettingsScreen(Camera* camera, GLFWwindow* window);
    ~SettingsScreen() override;    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput(float deltaTime) override;
    void onResize(int width, int height) override;

private:
    void layoutUI();
    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh);
    
    // UI buttons
    std::shared_ptr<Rendering::Components::Button> backButton;
    std::shared_ptr<Rendering::Components::Button> saveButton;
    
    float lastCursorX;
    float lastCursorY;
    
    // UI Layers
    std::shared_ptr<Rendering::Layer> backgroundLayer;
    std::shared_ptr<Rendering::Layer> controlsLayer;
    std::shared_ptr<Rendering::Layer> buttonLayer;
};