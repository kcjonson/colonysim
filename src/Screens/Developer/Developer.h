#pragma once

#include "../Screen.h"
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "../../Rendering/Layer.h"
#include "../../Rendering/Shapes/Rectangle.h"
#include "../../Rendering/Shapes/Text.h"
#include "../MainMenu/MainMenu.h" // For MenuButton structure

class DeveloperScreen : public Screen {
public:
    // Update constructor to accept Camera* and GLFWwindow*
    DeveloperScreen(Camera* camera, GLFWwindow* window);
    ~DeveloperScreen() override;    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput(float deltaTime) override;
    void onResize(int width, int height) override;

private:
    void layoutUI();
    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh);
    
    std::vector<MenuButton> buttons;
    float lastCursorX;
    float lastCursorY;
    
    // UI Layers
    std::shared_ptr<Rendering::Layer> backgroundLayer;
    std::shared_ptr<Rendering::Layer> buttonLayer;
    std::shared_ptr<Rendering::Layer> titleLayer;
};