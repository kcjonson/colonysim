#include <iostream>
#include "Screens/ScreenManager.h"
#include "GameState.h"

int main() {
    try {
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