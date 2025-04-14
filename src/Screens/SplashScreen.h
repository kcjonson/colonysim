#pragma once

#include "../Screen.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <chrono>
#include <memory>
#include "../Rendering/Layer.h"
#include "../Rendering/Shapes/Text.h"

class SplashScreen : public Screen {
public:
    SplashScreen();
    ~SplashScreen() override;

    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput() override;
    void onResize(int width, int height) override;

private:
    std::chrono::time_point<std::chrono::steady_clock> startTime;
    float displayDuration;
    std::string title;
    bool openglInitialized;
    bool clicked;
    
    // Layer for splash screen content
    std::shared_ptr<Rendering::Layer> splashLayer;
    std::shared_ptr<Rendering::Shapes::Text> titleText;
};