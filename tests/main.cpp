#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

int main(int argc, char* argv[]) {
    // Global setup can be done here if needed
    
    int result = Catch::Session().run(argc, argv);
    
    // Global cleanup can be done here if needed
    
    return result;
}

// This file acts as the main entry point for the Catch2 test framework.
//
// By using CATCH_CONFIG_RUNNER instead of CATCH_CONFIG_MAIN, we manually
// define the main() function and can add setup/teardown code if needed.
//
// Keep this file as simple as possible, since it will be compiled
// each time any test changes. Test helpers should be defined in other files.