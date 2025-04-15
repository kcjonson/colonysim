#include <iostream>
#include "Screens/ScreenManager.h"

int main() {
    try {
        // Create and initialize the screen manager
        ScreenManager screenManager;
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