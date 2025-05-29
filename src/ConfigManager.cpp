#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// Template function to parse string values to different types
template<typename T>
T parseValue(const std::string& value) {
    if constexpr (std::is_same_v<T, int>) {
        return std::stoi(value);
    } else if constexpr (std::is_same_v<T, float>) {
        return std::stof(value);
    } else if constexpr (std::is_same_v<T, unsigned int>) {
        return static_cast<unsigned int>(std::stoul(value));
    } else if constexpr (std::is_same_v<T, std::string>) {
        return value;
    } else {
        static_assert(false, "Unsupported type for parseValue");
    }
}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

// Helper function to parse nested JSON path (e.g., "window.width")
json getValue(const json& config, const std::string& path) {
    std::istringstream iss(path);
    std::string token;
    json current = config;
    
    while (std::getline(iss, token, '.')) {
        if (current.contains(token)) {
            current = current[token];
        } else {
            return json(); // Return null if path doesn't exist
        }
    }
    return current;
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

        // Load all config properties dynamically
#define CONFIG_PROP(type, name, defaultValue, path) \
        { \
            auto value = getValue(config, path); \
            if (!value.is_null()) { \
                try { \
                    name##_ = value.get<type>(); \
                    std::cout << "Loaded " << #name << " = " << name##_ << std::endl; \
                } catch (const std::exception& e) { \
                    std::cerr << "Error parsing " << path << ": " << e.what() << std::endl; \
                } \
            } \
        }

#define CONFIG_PROP_OPTIONAL(type, name, path) \
        { \
            auto value = getValue(config, path); \
            if (!value.is_null()) { \
                try { \
                    name##_ = value.get<type>(); \
                    std::cout << "Loaded " << #name << " = " << *name##_ << std::endl; \
                } catch (const std::exception& e) { \
                    std::cerr << "Error parsing " << path << ": " << e.what() << std::endl; \
                } \
            } else { \
                name##_ = std::nullopt; \
                std::cout << "Loaded " << #name << " = null (will use random)" << std::endl; \
            } \
        }

        CONFIG_PROPERTIES

#undef CONFIG_PROP
#undef CONFIG_PROP_OPTIONAL

        configLoaded = true;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

void ConfigManager::applyCommandLineOverrides(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '-' && i + 1 < argc) {
            std::string option = argv[i] + 2; // Skip the "--"
            std::string value = argv[i + 1];
            
            // Apply overrides dynamically
#define CONFIG_PROP(type, name, defaultValue, path) \
            if (option == #name) { \
                try { \
                    name##_ = parseValue<type>(value); \
                    std::cout << "Command line override: " << #name << " = " << name##_ << std::endl; \
                } catch (const std::exception& e) { \
                    std::cerr << "Error parsing command line value for " << #name << ": " << e.what() << std::endl; \
                } \
                i++; \
                continue; \
            }

#define CONFIG_PROP_OPTIONAL(type, name, path) \
            if (option == #name) { \
                try { \
                    name##_ = parseValue<type>(value); \
                    std::cout << "Command line override: " << #name << " = " << *name##_ << std::endl; \
                } catch (const std::exception& e) { \
                    std::cerr << "Error parsing command line value for " << #name << ": " << e.what() << std::endl; \
                } \
                i++; \
                continue; \
            }

            CONFIG_PROPERTIES

#undef CONFIG_PROP
#undef CONFIG_PROP_OPTIONAL
        }
    }
}