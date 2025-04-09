#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& filepath) {
    if (configLoaded) return true;
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << filepath << std::endl;
            return false;
        }

        json config = json::parse(file);

        // Validate JSON structure
        if (!config.is_object()) {
            std::cerr << "Invalid JSON structure: expected an object." << std::endl;
            return false;
        }

        // Load window settings
        if (config.contains("window")) {
            const auto& window = config["window"];
            if (window.contains("width")) windowWidth = window["width"].get<int>();
            if (window.contains("height")) windowHeight = window["height"].get<int>();
            if (window.contains("title")) windowTitle = window["title"].get<std::string>();
        }

        // Load camera settings
        if (config.contains("camera")) {
            const auto& camera = config["camera"];
            if (camera.contains("viewHeight")) viewHeight = camera["viewHeight"].get<float>();
            if (camera.contains("nearPlane")) nearPlane = camera["nearPlane"].get<float>();
            if (camera.contains("farPlane")) farPlane = camera["farPlane"].get<float>();
        }
        configLoaded = true;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
    
}