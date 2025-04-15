#pragma once

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
    virtual void handleInput() = 0;
    virtual void onResize(int width, int height) = 0;

protected:
    ScreenManager* screenManager; // Reference to the screen manager that owns this screen
};