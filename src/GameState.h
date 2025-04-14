#pragma once
#include <unordered_map>
#include <string>
#include <any> // For flexible data types
#include <iostream>

// Make GameState consistently a struct across the codebase
struct GameState {
    std::unordered_map<std::string, std::string> data;

    // Set string value
    void set(const std::string& key, const std::string& value) {
        data[key] = value;
    }

    // Get string value
    std::string get(const std::string& key) const {
        auto it = data.find(key);
        return it != data.end() ? it->second : "N/A";
    }

    template<typename T>
    T get(const std::string& key, const T& defaultValue) const {
        try {
            return std::any_cast<T>(data.at(key));
        } catch (...) {
            return defaultValue;
        }
    }
};
