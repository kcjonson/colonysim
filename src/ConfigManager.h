#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <variant>
#include <cctype>

// Helper macro to capitalize first letter
#define CAP_FIRST(name) \
    []() { \
        std::string s = #name; \
        if (!s.empty()) s[0] = std::toupper(s[0]); \
        return s; \
    }()

// Define all config properties - use CapitalCase for getter names
#define CONFIG_PROPERTIES \
    CONFIG_PROP(int, WindowWidth, 800, "window.width") \
    CONFIG_PROP(int, WindowHeight, 600, "window.height") \
    CONFIG_PROP(std::string, WindowTitle, "Colony Sim", "window.title") \
    CONFIG_PROP(float, ViewHeight, 1000.0f, "camera.viewHeight") \
    CONFIG_PROP(float, NearPlane, -1000.0f, "camera.nearPlane") \
    CONFIG_PROP(float, FarPlane, 1000.0f, "camera.farPlane") \
    CONFIG_PROP_OPTIONAL(unsigned int, DefaultSeed, "worldGeneration.defaultSeed") \
    CONFIG_PROP(int, ChunkSize, 1000, "world.chunkSize") \
    CONFIG_PROP(float, TileSize, 20.0f, "world.tileSize") \
    CONFIG_PROP(float, TilesPerMeter, 1.0f, "world.tilesPerMeter") \
    CONFIG_PROP(int, PreloadRadius, 1, "world.preloadRadius") \
    CONFIG_PROP(int, UnloadRadius, 2, "world.unloadRadius") \
    CONFIG_PROP(int, MaxLoadedChunks, 9, "world.maxLoadedChunks") \
    CONFIG_PROP(int, NumChunksToKeep, 25, "world.numChunksToKeep") \
    CONFIG_PROP(int, MaxNewTilesPerFrame, 100, "world.maxNewTilesPerFrame") \
    CONFIG_PROP(int, TileCullingOverscan, 3, "world.tileCullingOverscan") \
    CONFIG_PROP(int, TileSampleRate, 4, "world.tileSampleRate") \
    CONFIG_PROP(int, ChunkEdgeTriggerDistance, 10, "world.chunkEdgeTriggerDistance")

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadConfig(const std::string& filepath);
    void applyCommandLineOverrides(int argc, char* argv[]);

    // Generate getters - the property names should already be camelCase
#define CONFIG_PROP(type, name, defaultValue, path) \
    type get##name() const { return name##_; }

#define CONFIG_PROP_OPTIONAL(type, name, path) \
    std::optional<type> get##name() const { return name##_; }

    CONFIG_PROPERTIES

#undef CONFIG_PROP
#undef CONFIG_PROP_OPTIONAL

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    bool configLoaded = false;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Generate member variables using macros (convert CapitalCase back to camelCase)
#define CONFIG_PROP(type, name, defaultValue, path) \
    type name##_ = defaultValue;

#define CONFIG_PROP_OPTIONAL(type, name, path) \
    std::optional<type> name##_;

    CONFIG_PROPERTIES

#undef CONFIG_PROP
#undef CONFIG_PROP_OPTIONAL
}; 