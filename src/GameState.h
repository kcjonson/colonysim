#pragma once
#include <unordered_map>
#include <string>
#include <any> // For flexible data types
#include <iostream>
#include <mutex>
#include <shared_mutex>

// Make GameState consistently a struct across the codebase
struct GameState {
private:
    std::unordered_map<std::string, std::string> data;
    mutable std::shared_mutex mutex; // Thread safety for concurrent access
    bool initialized = false;

public:
    GameState() {
        initialized = true;
    }

    // Set string value - thread-safe and null-safe
    void set(const std::string& key, const std::string& value) {
        if (key.empty()) return;
        
        try {
            std::unique_lock lock(mutex); // Write lock
            data[key] = value;
        }
        catch (const std::exception& e) {
            std::cerr << "Error in GameState::set: " << e.what() << std::endl;
        }
    }

    // Get string value - thread-safe and null-safe
    // Always returns a valid string (never crashes)
    std::string get(const std::string& key) const {
        if (key.empty()) return "N/A";
        
        try {
            std::shared_lock lock(mutex); // Read lock
            auto it = data.find(key);
            return it != data.end() ? it->second : "N/A";
        }
        catch (const std::exception& e) {
            std::cerr << "Error in GameState::get: " << e.what() << std::endl;
            return "Error";
        }
    }

    // Template get with default value - thread-safe and null-safe
    template<typename T>
    T get(const std::string& key, const T& defaultValue) const {
        if (key.empty()) return defaultValue;
        
        try {
            std::shared_lock lock(mutex); // Read lock
            auto it = data.find(key);
            return it != data.end() ? std::any_cast<T>(it->second) : defaultValue;
        } 
        catch (...) {
            return defaultValue;
        }
    }

    // Check if a key exists - thread-safe
    bool hasKey(const std::string& key) const {
        if (key.empty()) return false;
        
        try {
            std::shared_lock lock(mutex); // Read lock
            return data.find(key) != data.end();
        }
        catch (...) {
            return false;
        }
    }

    // Remove a key - thread-safe
    void remove(const std::string& key) {
        if (key.empty()) return;
        
        try {
            std::unique_lock lock(mutex); // Write lock
            data.erase(key);
        }
        catch (const std::exception& e) {
            std::cerr << "Error in GameState::remove: " << e.what() << std::endl;
        }
    }
    
    // Clear all keys - thread-safe
    void clear() {
        try {
            std::unique_lock lock(mutex); // Write lock
            data.clear();
        }
        catch (const std::exception& e) {
            std::cerr << "Error in GameState::clear: " << e.what() << std::endl;
        }
    }
};
