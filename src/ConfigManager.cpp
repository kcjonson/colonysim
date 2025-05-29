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
        
        // Load world settings
        if (config.contains("world")) {
            const auto& world = config["world"];
            if (world.contains("chunkSize")) chunkSize = world["chunkSize"].get<int>();
            if (world.contains("tileSize")) tileSize = world["tileSize"].get<float>();
            if (world.contains("tilesPerMeter")) tilesPerMeter = world["tilesPerMeter"].get<float>();
            if (world.contains("preloadRadius")) preloadRadius = world["preloadRadius"].get<int>();
            if (world.contains("unloadRadius")) unloadRadius = world["unloadRadius"].get<int>();
            if (world.contains("maxLoadedChunks")) maxLoadedChunks = world["maxLoadedChunks"].get<int>();
            if (world.contains("maxNewTilesPerFrame")) maxNewTilesPerFrame = world["maxNewTilesPerFrame"].get<int>();
            if (world.contains("tileCullingOverscan")) tileCullingOverscan = world["tileCullingOverscan"].get<int>();
            if (world.contains("tileSampleRate")) tileSampleRate = world["tileSampleRate"].get<int>();
            if (world.contains("chunkEdgeTriggerDistance")) chunkEdgeTriggerDistance = world["chunkEdgeTriggerDistance"].get<int>();
            if (world.contains("numChunksToKeep")) numChunksToKeep = world["numChunksToKeep"].get<int>();
        }
        
        configLoaded = true;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
    
}