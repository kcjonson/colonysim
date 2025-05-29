#include <iostream>
#include <string>
#include <cstring>
#include "Screens/ScreenManager.h"
#include "GameState.h"
#include "ConfigManager.h"

int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments and apply config overrides
        for (int i = 1; i < argc; i++) {
            if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
                std::cout << "ColonySim Usage:" << std::endl;
                std::cout << "  --worldSeed <seed>     Override default world generation seed" << std::endl;
                std::cout << "  --windowWidth <width>  Override window width" << std::endl;
                std::cout << "  --windowHeight <height> Override window height" << std::endl;
                std::cout << "  --help, -h             Show this help message" << std::endl;
                return 0;
            }
        }
        
        // Load config first
        auto& config = ConfigManager::getInstance();
        if (!config.loadConfig("config/game_config.json")) {
            std::cerr << "Warning: Could not load config file, using defaults" << std::endl;
        }
        
        // Apply command line overrides to config
        config.applyCommandLineOverrides(argc, argv);
        
        // Create the GameState
        GameState* gameState = new GameState();
        if (!gameState) {
            std::cerr << "Failed to create GameState" << std::endl;
            return 1;
        }

        // Create and initialize the screen manager with our GameState
        ScreenManager screenManager(gameState);
        if (!screenManager.initialize()) {
            std::cerr << "Failed to initialize ScreenManager" << std::endl;
            return 1;
        }
        
        // Run the game with the screen manager
        screenManager.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}