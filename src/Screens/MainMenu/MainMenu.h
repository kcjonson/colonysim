#pragma once

#include "../Screen.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <glm/glm.hpp>
#include "../../Rendering/Layer.h"
#include "../../Rendering/Shapes/Rectangle.h"
#include "../../Rendering/Shapes/Text.h"
#include "../../Rendering/Components/Button.h"


// DANGER: THis is used elsewhere. 
// TODO: REMOVE ONCE FULLY MIGRATED TO BUTTON CLASS
struct MenuButton {
    std::string text;
    glm::vec2 position;
    glm::vec2 size;
    glm::vec4 color;
    glm::vec4 hoverColor;
    std::function<void()> callback;
    bool isHovered;
    
    // References to the actual shape objects
    std::shared_ptr<Rendering::Shapes::Rectangle> background;
    std::shared_ptr<Rendering::Shapes::Text> label;
};

class MainMenuScreen : public Screen {
public:
    // Update constructor to accept Camera and Window pointers
    MainMenuScreen(Camera* camera, GLFWwindow* window);
    ~MainMenuScreen() override;

    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput(float deltaTime) override;
    void onResize(int width, int height) override;

private:

    int menuWidth = 300;
    int menuPadding = 20;
    int buttonSpacing = 10;

    // UI Layers
    std::shared_ptr<Rendering::Layer> backgroundLayer;
    std::shared_ptr<Rendering::Layer> buttonLayer;
    std::shared_ptr<Rendering::Layer> titleLayer;

    std::shared_ptr<Rendering::Shapes::Rectangle> menuBackground;
    std::shared_ptr<Rendering::Components::Button> newColonyButton;
    std::shared_ptr<Rendering::Components::Button> loadColonyButton;
    std::shared_ptr<Rendering::Components::Button> settingsButton;
    std::shared_ptr<Rendering::Components::Button> developerButton;

    void doLayout();
};