#pragma once

#include "../CoordinateSystem.h"

// Forward declarations
class ScreenManager;

class Screen {
public:
    Screen() : screenManager(nullptr) {}
    virtual ~Screen() = default;

    // Set the screen manager reference
    void setScreenManager(ScreenManager* manager) { screenManager = manager; }

    // Virtual methods to be implemented by derived screen classes
    virtual bool initialize() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    virtual void handleInput(float deltaTime) = 0;
    virtual void onResize(int width, int height) = 0;
    
    // Called when leaving this screen - resets OpenGL state
    virtual void onExit() {
        auto& coordSys = CoordinateSystem::getInstance();
        coordSys.resetOpenGLState();
    }

protected:
    ScreenManager* screenManager; // Reference to the screen manager that owns this screen
};