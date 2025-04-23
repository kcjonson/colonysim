#include <iostream>
#include "Screens/ScreenManager.h"
#include "GameState.h"

int main() {
    try {
        // Create and initialize GameState first, before anything else
        GameState* gameState = new GameState();
        if (!gameState) {
            std::cerr << "Failed to create GameState" << std::endl;
            return 1;
        }
        
        // Pre-initialize key GameState values to prevent access violations
        try {
            gameState->set("system.version", "0.1.0");
            gameState->set("system.fps", "0");
            gameState->set("world.totalTiles", "0");
            gameState->set("world.shownTiles", "0");
            gameState->set("world.totalShapes", "0");
            gameState->set("world.tileMemKB", "0");
            gameState->set("world.shapeMemKB", "0");
            gameState->set("world.totalMemKB", "0");
            gameState->set("input.windowPos", "0, 0");
            gameState->set("input.worldPos", "0, 0");
            gameState->set("camera.position", "0, 0");
            gameState->set("rend.vertices", "0");
            gameState->set("rend.indices", "0");
            std::cout << "GameState initialized with default values" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Warning: Failed to pre-initialize GameState values: " << e.what() << std::endl;
            // Continue anyway, the ScreenManager will initialize these values
        }

        // Create and initialize the screen manager with our pre-initialized GameState
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