#pragma once

#include <string>

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadConfig(const std::string& filepath);
    
    // Window settings
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }
    const std::string& getWindowTitle() const { return windowTitle; }
    
    // Camera settings
    float getViewHeight() const { return viewHeight; }
    float getNearPlane() const { return nearPlane; }
    float getFarPlane() const { return farPlane; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    bool configLoaded = false;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Window settings
    int windowWidth = 800;
    int windowHeight = 600;
    std::string windowTitle = "Colony Sim";

    // Camera settings
    float viewHeight = 1000.0f;
    float nearPlane = -1000.0f;
    float farPlane = 1000.0f;
}; 