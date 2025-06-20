#include "WorldGen.h"
#include <iostream>
#include "../ScreenManager.h"
#include "../Game/World.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>
#include <cmath>
#include "Generators/TerrainGenerator.h"
#include "Core/Util.h" // Include the new Util.h file
#include "VectorGraphics.h"
#include <algorithm> // Keep algorithm include
#include "../../CoordinateSystem.h"

// Define M_PI if not already defined
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Initialize the static instances map
std::unordered_map<GLFWwindow*, WorldGenScreen*> WorldGenScreen::instances;

// Update constructor definition to accept Camera* and GLFWwindow*
WorldGenScreen::WorldGenScreen(Camera* camera, GLFWwindow* window): 
    lastCursorX(0.0f)
    , lastCursorY(0.0f)
    , worldWidth(256)
    , worldHeight(256)
    , waterLevel(0.4f)
    , worldGenerated(false)
    , cameraDistance(5.0f)
    , rotationAngle(0.0f)
    , isDragging(false)
    , window(window)
    , currentVisualizationMode(WorldGen::VisualizationMode::Terrain) {
    
    // Register this instance in the static map
    instances[window] = this;
      // Create a central progress tracker with a callback to update the UI
    progressTracker = std::make_shared<WorldGen::ProgressTracker>();
    
    // Initialize Stars
    stars = std::make_unique<WorldGen::Stars>(camera, window);
    
    // Initialize WorldGenUI
    worldGenUI = std::make_unique<WorldGen::WorldGenUI>(camera, window);

    planetParams = WorldGen::PlanetParameters();

    // Planet parameters are initialized with defaults
    // Seed is now managed separately through the UI
}

WorldGenScreen::~WorldGenScreen() {
    // Signal threads to stop
    shouldStopGeneration = true;
    shouldStopGameWorldCreation = true;
    
    // Wait for threads to finish
    if (generationThread.joinable()) {
        generationThread.join();
    }
    
    if (gameWorldThread.joinable()) {
        gameWorldThread.join();
    }
    
    // Remove scroll callback
    if (window) {
        glfwSetScrollCallback(window, nullptr);
        // Remove this instance from the static map
        instances.erase(window);
    }
}

bool WorldGenScreen::initialize() {
    // Initialize UI
    if (!worldGenUI->initialize()) {
        std::cerr << "Failed to initialize world gen UI" << std::endl;
        return false;
    }
    
    // Set up the progress tracker callback now that UI is initialized
    progressTracker->SetCallback([this](float progress, const std::string& message) {
        std::lock_guard<std::mutex> lock(progressMutex);
        latestProgress.progress = progress;
        latestProgress.message = message;
        latestProgress.hasUpdate = true;
    });
    
    // Set up OpenGL blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up scroll callback without overriding window user pointer
    glfwSetScrollCallback(window, scrollCallback);    // Initialize world object and renderer, but don't generate the world yet
    world = std::make_unique<WorldGen::Generators::World>(planetParams, 12345, progressTracker);
    worldRenderer = std::make_unique<WorldGen::Renderers::World>();
    landingLocation = std::make_unique<WorldGen::Renderers::LandingLocation>(worldRenderer.get());
    
    std::cout << "Initializing world renderer and landing location..." << std::endl;
      worldRenderer->SetWorld(world.get());
    landingLocation->SetWorld(world.get());
    
    // No longer generate a dummy location - we'll use the cursor
    // landingLocation->GenerateDummyLocation();
    
    std::cout << "World renderer and landing location initialized" << std::endl;

    // Initialize the projection matrix
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    projectionMatrix = glm::perspective(glm::radians(45.0f), static_cast<float>(windowWidth) / windowHeight, 0.1f, 100.0f);
      // Initialize the view matrix
    // Adjust camera distance based on world radius (if world is generated)
    cameraDistance = 2.5f; // Default camera distance
    viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, cameraDistance),  // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f),              // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)               // Up vector
    );      // Register event handlers for UI events

    // Generate World button event
    worldGenUI->addEventListener(WorldGen::UIEvent::GenerateWorld, [this]() {
        std::cout << "Generate World button clicked" << std::endl;
        // Switch to generating state
        worldGenUI->setState(WorldGen::UIState::Generating);
        
        // Get seed from UI (seed is no longer part of PlanetParameters)
        currentSeed = worldGenUI->getCurrentSeed();
        
        // Reset the progress tracker
        progressTracker->Reset();
        
        // Stop any existing generation thread
        if (isGenerating) {
            shouldStopGeneration = true;
            if (generationThread.joinable()) {
                generationThread.join();
            }
            shouldStopGeneration = false;
        }
        
        // Start a new generation thread
        isGenerating = true;
        generationThread = std::thread(&WorldGenScreen::worldGenerationThreadFunc, this);
    });
    
    // Visualization mode change event
    worldGenUI->addEventListener(WorldGen::UIEvent::ChangeVisualization, [this]() {
        currentVisualizationMode = worldGenUI->getVisualizationMode();
        std::cout << "Visualization mode changed to: " << static_cast<int>(currentVisualizationMode) << std::endl;
        // The tile coloring will be handled in the render loop
    });
    
    // Land button event
    worldGenUI->addEventListener(WorldGen::UIEvent::GoToLand, [this]() {
        if (!worldGenerated) {
            // If world is not generated, log an error message and return early
            std::cerr << "ERROR: World must be generated before proceeding to land view" << std::endl;
            worldGenUI->setState(WorldGen::UIState::ParameterSetup);
            return;
        }        std::cout << "Land on World button clicked" << std::endl;
        
        // Don't start a new thread if we're already creating a game world
        if (isCreatingGameWorld) {
            std::cout << "Already creating game world, please wait..." << std::endl;
            return;
        }
        
        // Create a new game world from the generator world in a background thread
        // Use the progress tracker directly
        progressTracker->Reset();
        progressTracker->AddPhase("Converting", 0.8f);
        progressTracker->AddPhase("Finalizing", 0.2f);
        
        // Calculate sample rate based on world complexity - higher subdivisions need higher sampling
        float subdivisionLevel = static_cast<float>(planetParams.resolution);
        float worldRadius = world->GetRadius();
        
        // More aggressive clamping of subdivision level
        // A subdivision level of 5 is already quite detailed and produces a high-quality map
        float clampedSubdivision = std::min(subdivisionLevel, 4.0f); 
        
        // Adjusted sample rate calculation to be more conservative
        // This creates a more reasonable number of tiles while maintaining quality
        float sampleRate = 6.0f * (1.0f + clampedSubdivision);
        
        // Add a hard maximum on the sample rate to prevent performance and memory issues
        const float MAX_SAMPLE_RATE = 48.0f;
        sampleRate = std::min(sampleRate, MAX_SAMPLE_RATE);
        
        std::cout << "Using sample rate: " << sampleRate << " for planet with subdivision level " 
                  << subdivisionLevel << " (clamped to " << clampedSubdivision << ") "
                  << "and radius " << worldRadius << std::endl;
        
        // Get components we need from the screen manager
        GameState* gameState = screenManager->getGameState();
        Camera* camera = screenManager->getCamera();
        
        // Check if required components are available
        if (!gameState || !camera) {
            std::cerr << "ERROR: Required components missing from screen manager" << std::endl;
            return;
        }
        
        // Set up the parameters for the game world creation thread
        gameWorldParams.sampleRate = sampleRate;
        gameWorldParams.camera = camera;
        gameWorldParams.gameState = gameState;
        gameWorldParams.window = window;
        gameWorldParams.seed = std::to_string(currentSeed);
        
        // Set flag to indicate we're creating a game world
        isCreatingGameWorld = true;
        
        // Show loading UI
        worldGenUI->setState(WorldGen::UIState::LoadingGameWorld);
        
        // Start the game world creation thread
        std::cout << "Starting game world creation thread..." << std::endl;
        gameWorldThread = std::thread(&WorldGenScreen::gameWorldCreationThreadFunc, this);
    });
    
    // Back button event
    worldGenUI->addEventListener(WorldGen::UIEvent::Back, [this]() {
        // Reset OpenGL state before switching screens
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glViewport(0, 0, width, height);  // Reset to full window viewport
        glDisable(GL_DEPTH_TEST);         // Disable depth testing
        glEnable(GL_BLEND);               // Ensure blending is enabled
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set standard alpha blending
        glLineWidth(1.0f);                // Reset line width to default
        
        screenManager->switchScreen(ScreenType::MainMenu);
    });
    
    // Set initial UI state
    worldGenUI->setState(WorldGen::UIState::ParameterSetup);
      // Set initial UI layout
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    stars->generate(width, height);
    worldGenUI->onResize(width, height);
    
    return true;
}

void WorldGenScreen::update(float deltaTime) {
    // Process any pending progress messages
    processProgressMessages();
    
    // Rest of existing update code...
    // If world is generated, adjust camera distance based on world radius
    if (worldGenerated && world) {
        // Set the camera distance based on the world's radius (add a margin for better visibility)
        float worldRadius = world->GetRadius();
        cameraDistance = worldRadius * 2.5f;
    }

    // Update camera matrices
    viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, cameraDistance),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // Update model-view matrix with current rotation
    glm::mat4 rotationMatrix = glm::rotate(
        glm::mat4(1.0f),
        rotationAngle,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    viewMatrix = viewMatrix * rotationMatrix;
    
    // Update projection matrix - use full width for proper aspect ratio
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(width) / height,
        0.1f,
        100.0f
    );

    worldGenUI->update(deltaTime);
}

void WorldGenScreen::render() {
    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Get window size for UI calculations (logical pixels)
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Get framebuffer size for viewport (physical pixels)
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    
    // Use the full window viewport for all rendering
    glViewport(0, 0, fbWidth, fbHeight);
    
    // --- Render stars (background, always first, blending enabled) ---
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    stars->render();    // --- Render the icosahedron world if generated ---

    if (worldGenerated && world && worldRenderer) {
        // Enable depth testing for 3D rendering
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Only render in the main area (not over sidebar)
        float sidebarWidth = worldGenUI->getSidebarWidth();
        // Convert logical sidebar width to physical pixels using pixel ratio
        auto& coordSys = CoordinateSystem::getInstance();
        float pixelRatio = coordSys.getPixelRatio();
        int fbSidebarWidth = static_cast<int>(sidebarWidth * pixelRatio);
        int renderWidth = fbWidth - fbSidebarWidth;
        int renderHeight = fbHeight;
        glViewport(fbSidebarWidth, 0, renderWidth, renderHeight);
        
        // Calculate correct aspect ratio for the viewport
        float aspectRatio = static_cast<float>(renderWidth) / renderHeight;
        
        // Create adjusted projection matrix for this viewport
        glm::mat4 adjustedProjection = glm::perspective(
            glm::radians(45.0f),  // FOV
            aspectRatio,          // Aspect ratio
            0.1f,                 // Near plane
            100.0f                // Far plane
        );        // Update renderer with current visualization mode
        worldRenderer->SetVisualizationMode(currentVisualizationMode);
        
        // Pass plate data if available
        const auto& plates = world->GetPlates();
        if (!plates.empty()) {
            worldRenderer->SetPlateData(plates);
        }
        
        // Render the world with adjusted projection
        worldRenderer->Render(viewMatrix, adjustedProjection);
        
        // Render the landing location indicator
        if (landingLocation) {
            // std::cout << "Rendering landing location indicator" << std::endl;
            landingLocation->Render(viewMatrix, adjustedProjection);
        }
        
        // Fully reset OpenGL state for UI rendering
        glViewport(0, 0, fbWidth, fbHeight);
        glDisable(GL_DEPTH_TEST);
        
        // Make sure blending is properly set up again for UI rendering
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        
        // Unbind any active shader program
        glUseProgram(0);
    }

    // --- Render UI layers ---
    worldGenUI->render();
}

void WorldGenScreen::handleInput(float deltaTime) {
    // Get mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float mouseX = static_cast<float>(xpos);
    float mouseY = static_cast<float>(ypos);
    
    // Only process mouse input if cursor is in the planet view area (right side of screen)
    if (mouseX > worldGenUI->getSidebarWidth()) {
          // Handle mouse clicks
        bool isLeftButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        static bool wasLeftButtonPressed = false;
        static float mouseDownX = 0.0f; // Track where the mouse was first pressed
        static float mouseDownY = 0.0f;
        static bool hasDragged = false;
          // If we have a world and landing location renderer...
        if (landingLocation && world && worldGenerated) {
            int width, height;
            glfwGetWindowSize(window, &width, &height);
          // Always update landing location based on mouse position
            // if a location hasn't been selected yet
            if (!landingLocation->HasLocationSelected()) {                // Adjust mouse coordinates for the sidebar offset
                float sidebarWidth = worldGenUI->getSidebarWidth();
                float adjustedMouseX = mouseX - sidebarWidth; // Subtract sidebar width
                
                // Calculate viewport dimensions
                int renderWidth = width - static_cast<int>(sidebarWidth);
                int renderHeight = height;
                  // Calculate correct aspect ratio for the viewport
                float aspectRatio = static_cast<float>(renderWidth) / renderHeight;
                
                // Create adjusted projection matrix for this viewport
                glm::mat4 adjustedProjection = glm::perspective(
                    glm::radians(45.0f),  // FOV
                    aspectRatio,          // Aspect ratio
                    0.1f,                 // Near plane
                    100.0f                // Far plane
                );
                
                landingLocation->UpdateFromMousePosition(
                    adjustedMouseX, mouseY, 
                    viewMatrix, adjustedProjection, // Use the same adjusted projection as for rendering 
                    renderWidth, renderHeight
                );
            }
            
            // When the mouse button is first pressed, record the position
            if (isLeftButtonPressed && !wasLeftButtonPressed) {
                mouseDownX = mouseX;
                mouseDownY = mouseY;
                hasDragged = false;
                
                // Start potential dragging for planet rotation
                isDragging = true;
                lastCursorX = mouseX;
                lastCursorY = mouseY;
            }
            // If dragging, update rotation
            else if (isLeftButtonPressed && isDragging) {
                float deltaX = mouseX - lastCursorX;
                float deltaY = mouseY - lastCursorY;
                
                // If the mouse moves more than a small threshold, consider it a drag
                if (std::abs(deltaX) > 3.0f || std::abs(deltaY) > 3.0f) {
                    hasDragged = true;
                }
                
                rotationAngle += deltaX * 0.01f;
                lastCursorX = mouseX;
                lastCursorY = mouseY;
            }
            // If button released, handle click or end dragging
            else if (!isLeftButtonPressed && wasLeftButtonPressed) {
                // Button has just been released
                isDragging = false;
                
                // Only consider it a click if the mouse hasn't moved much (not a drag)
                float distMoved = std::sqrt(std::pow(mouseX - mouseDownX, 2) + std::pow(mouseY - mouseDownY, 2));
                
                if (!hasDragged && distMoved < 5.0f) {
                    // It's a click, not a drag
                    
                    // If we already have a location selected, unselect it
                    if (landingLocation->HasLocationSelected()) {
                        landingLocation->Reset();
                        worldGenUI->setState(WorldGen::UIState::Viewing);
                        std::cout << "Landing location unselected!" << std::endl;
                    }
                    // Otherwise try to select a new landing location
                    else if (landingLocation->SelectCurrentLocation()) {
                        worldGenUI->setState(WorldGen::UIState::Landing);
                        std::cout << "Landing location selected!" << std::endl;
                    }
                }
            }
        }
        // If no world/landing location but mouse pressed, handle just planet rotation
        else if (isLeftButtonPressed) {
            if (!isDragging) {
                isDragging = true;
                lastCursorX = mouseX;
                lastCursorY = mouseY;
            } else {
                float deltaX = mouseX - lastCursorX;
                rotationAngle += deltaX * 0.01f;
                lastCursorX = mouseX;
                lastCursorY = mouseY;
            }
        } else {
            isDragging = false;
        }
        
        wasLeftButtonPressed = isLeftButtonPressed;
    }
      // Check for ESC key to go back to main menu
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        screenManager->switchScreen(ScreenType::MainMenu);
    }
    
    // Check for 'R' key to reset landing location
    static bool wasRPressed = false;
    bool isRPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (isRPressed && !wasRPressed) {
        if (landingLocation) {
            landingLocation->Reset();
            std::cout << "Landing location reset!" << std::endl;
        }
    }
    wasRPressed = isRPressed;

    worldGenUI->handleInput(deltaTime);
}

void WorldGenScreen::onResize(int width, int height) {
    // Update UI layout
    worldGenUI->onResize(width, height);
    
    // Update projection matrix
    projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(width) / height,
        0.1f,
        100.0f
    );
}

bool WorldGenScreen::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

void WorldGenScreen::handleScroll(double xoffset, double yoffset) {
    // Adjust camera distance with scroll wheel (zoom in/out)
    cameraDistance -= static_cast<float>(yoffset) * 0.5f;
    
    // Clamp to reasonable limits
    cameraDistance = std::max(1.5f, std::min(10.0f, cameraDistance));
}

// Static callback that routes scroll events to the right instance
void WorldGenScreen::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto it = instances.find(window);
    if (it != instances.end()) {
        it->second->handleScroll(xoffset, yoffset);
    }
}

// Convert the icosahedron world to terrain data
void WorldGenScreen::convertWorldToTerrainData() {
    if (!world) {
        std::cerr << "ERROR: World not initialized for conversion" << std::endl;
        return;
    }
    
    // Clear previous terrain data
    generatedTerrainData.clear();
    
    // Get the tiles from the world
    const auto& tiles = world->GetTiles();
    
    // Counters for terrain types
    int waterTileCount = 0;
    int landTileCount = 0;
    int totalTileCount = 0;
    
    // Report starting conversion
    if (progressTracker) {
        progressTracker->UpdateProgress(0.1f, "Starting terrain conversion...");
    }
    
    // Track progress
    size_t totalTiles = tiles.size();
    
    // Project tiles onto a 2D grid based on spherical coordinates
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        const auto& center = tile.GetCenter();
        
        // Convert 3D position to longitude/latitude
        float longitude = std::atan2(center.z, center.x);
        float latitude = std::asin(center.y / world->GetRadius());
        
        // Convert longitude/latitude to grid coordinates
        int x = static_cast<int>((longitude / (2.0f * 3.14159f) + 0.5f) * worldWidth);
        int y = static_cast<int>((latitude / 3.14159f + 0.5f) * worldHeight);
        
        // Create terrain data
        WorldGen::TerrainData terrainData;
        
        // Assign terrain type based on some attributes of the tile
        // For example, use the elevation for terrain type
        float elevation = tile.GetElevation();
        
        if (elevation < waterLevel - 0.2f) {
            terrainData.type = WorldGen::TerrainType::Ocean;
            waterTileCount++;
        } else if (elevation < waterLevel - 0.05f) {
            terrainData.type = WorldGen::TerrainType::Shallow;
            waterTileCount++;
        } else if (elevation < waterLevel + 0.05f) {
            terrainData.type = WorldGen::TerrainType::Beach;
            landTileCount++;
        } else if (elevation < waterLevel + 0.3f) {
            terrainData.type = WorldGen::TerrainType::Lowland;
            landTileCount++;
        } else if (elevation < waterLevel + 0.6f) {
            terrainData.type = WorldGen::TerrainType::Highland;
            landTileCount++;
        } else if (elevation < waterLevel + 0.8f) {
            terrainData.type = WorldGen::TerrainType::Mountain;
            landTileCount++;
        } else {
            terrainData.type = WorldGen::TerrainType::Peak;
            landTileCount++;
        }
        
        totalTileCount++;
        
        // Set extra terrain data
        terrainData.elevation = elevation;
        terrainData.humidity = tile.GetMoisture();
        terrainData.temperature = tile.GetTemperature();
        terrainData.height = elevation; // Also set the legacy height field
        
        // Set a color based on terrain type
        switch (terrainData.type) {
            case WorldGen::TerrainType::Ocean:
                terrainData.color = glm::vec4(0.0f, 0.0f, 0.8f, 1.0f);
                break;
            case WorldGen::TerrainType::Shallow:
                terrainData.color = glm::vec4(0.0f, 0.4f, 0.8f, 1.0f);
                break;
            case WorldGen::TerrainType::Beach:
                terrainData.color = glm::vec4(0.9f, 0.9f, 0.7f, 1.0f);
                break;
            case WorldGen::TerrainType::Lowland:
                terrainData.color = glm::vec4(0.0f, 0.6f, 0.0f, 1.0f);
                break;
            case WorldGen::TerrainType::Highland:
                terrainData.color = glm::vec4(0.0f, 0.4f, 0.0f, 1.0f);
                break;
            case WorldGen::TerrainType::Mountain:
                terrainData.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                break;
            case WorldGen::TerrainType::Peak:
                terrainData.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                break;
            default:
                terrainData.color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta for unknown
        }
        
        // Store in the terrain data map
        generatedTerrainData[{x, y}] = terrainData;
        
        // Update progress every 1000 tiles
        if (progressTracker && i % 1000 == 0) {
            float progress = 0.1f + (static_cast<float>(i) / totalTiles) * 0.8f;
            std::string message = "Converting tiles to terrain data (" + 
                                 std::to_string(i) + " of " + 
                                 std::to_string(totalTiles) + ")";
            progressTracker->UpdateProgress(progress, message);
        }
    }
    
    // Signal completion
    if (progressTracker) {
        progressTracker->UpdateProgress(0.9f, "Analyzing terrain data...");
    }
    
    // Log tile statistics with more detailed information
    std::cout << "\n============ WORLD GENERATION STATS ============" << std::endl;
    std::cout << "Total tiles: " << totalTileCount << std::endl;
    std::cout << "Water tiles: " << waterTileCount << " (" << (static_cast<float>(waterTileCount) / totalTileCount * 100.0f) << "%)" << std::endl;
    std::cout << "Land tiles: " << landTileCount << " (" << (static_cast<float>(landTileCount) / totalTileCount * 100.0f) << "%)" << std::endl;
    std::cout << "Water level: " << waterLevel << std::endl;

    // Distribution of terrain types
    int oceanCount = 0, shallowCount = 0, beachCount = 0, lowlandCount = 0;
    int highlandCount = 0, mountainCount = 0, peakCount = 0;

    for (const auto& [coord, data] : generatedTerrainData) {
        switch (data.type) {
            case WorldGen::TerrainType::Ocean: oceanCount++; break;
            case WorldGen::TerrainType::Shallow: shallowCount++; break;
            case WorldGen::TerrainType::Beach: beachCount++; break;
            case WorldGen::TerrainType::Lowland: lowlandCount++; break;
            case WorldGen::TerrainType::Highland: highlandCount++; break;
            case WorldGen::TerrainType::Mountain: mountainCount++; break;
            case WorldGen::TerrainType::Peak: peakCount++; break;
            case WorldGen::TerrainType::Volcano: /* Count volcanoes separately if needed */ break;
        }
    }

    std::cout << "Terrain distribution:" << std::endl;
    std::cout << "  Ocean: " << oceanCount << " tiles (" << (static_cast<float>(oceanCount) / totalTileCount * 100.0f) << "%)" << std::endl;
    std::cout << "  Shallow: " << shallowCount << " tiles (" << (static_cast<float>(shallowCount) / totalTileCount * 100.0f) << "%)" << std::endl;
    std::cout << "  Beach: " << beachCount << " tiles (" << (static_cast<float>(beachCount) / totalTileCount * 100.0f) << "%)" << std::endl;
    std::cout << "  Lowland: " << lowlandCount << " tiles (" << (static_cast<float>(lowlandCount) / totalTileCount * 100.0f) << "%)" << std::endl;
    std::cout << "  Highland: " << highlandCount << " tiles (" << (static_cast<float>(highlandCount) / totalTileCount * 100.0f) << "%)" << std::endl;
    std::cout << "  Mountain: " << mountainCount << " tiles (" << (static_cast<float>(mountainCount) / totalTileCount * 100.0f) << "%)" << std::endl;
    std::cout << "  Peak: " << peakCount << " tiles (" << (static_cast<float>(peakCount) / totalTileCount * 100.0f) << "%)" << std::endl;
    std::cout << "Converted " << generatedTerrainData.size() << " tiles to terrain data" << std::endl;
    std::cout << "================================================\n" << std::endl;
    
    // Signal final completion
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Terrain conversion complete!");
    }
}

void WorldGenScreen::worldGenerationThreadFunc() {
    try {
        // Generate the world in this background thread
        world = WorldGen::Generators::Generator::CreateWorld(planetParams, currentSeed, progressTracker);
        
        // Check if we should stop
        if (shouldStopGeneration) {
            isGenerating = false;
            return;
        }
        
        // The complete world generation pipeline is now handled by Generator::CreateWorld
        // which includes geometry, plates, mountains, and future features
        
        // Generation complete
        worldGenerated = true;
        
        // Set final completion progress
        std::lock_guard<std::mutex> lock(progressMutex);
        latestProgress.progress = 1.0f;
        latestProgress.message = "World generation complete!";
        latestProgress.hasUpdate = true;
    }
    catch (const std::exception& e) {
        // Handle any exceptions
        std::lock_guard<std::mutex> lock(progressMutex);
        latestProgress.progress = 0.0f;
        latestProgress.message = std::string("Error: ") + e.what();
        latestProgress.hasUpdate = true;
    }
    
    isGenerating = false;
}

// New method for creating the game world in a background thread
void WorldGenScreen::gameWorldCreationThreadFunc() {
    try {
        // Update progress tracker
        progressTracker->StartPhase("Converting");
        
        // Get the parameters
        float sampleRate = gameWorldParams.sampleRate;
        Camera* camera = gameWorldParams.camera;
        GameState* gameState = gameWorldParams.gameState;
        GLFWwindow* window = gameWorldParams.window;
        std::string seed = gameWorldParams.seed;
        
        // Check if we should stop
        if (shouldStopGameWorldCreation) {
            std::lock_guard<std::mutex> lock(progressMutex);
            latestProgress.progress = 0.0f;
            latestProgress.message = "Game world creation canceled";
            latestProgress.hasUpdate = true;
            isCreatingGameWorld = false;
            return;
        }
        
        // Get landing location from the landing location renderer
        glm::vec3 landingPos = landingLocation->GetSelectedLocation();
        
        // Generate initial chunk at landing location
        auto initialChunk = WorldGen::Core::generateInitialChunk(*world, landingPos);
        
        // Create the new World with chunked loading
        newWorld = std::make_unique<World>(
            *gameState,       // Game state
            seed,             // Seed
            camera,           // Camera
            window,           // Window
            world.get(),    // Spherical world reference
            std::move(initialChunk), // Initial chunk
            landingPos   // Landing location
        );
        
        // Check if we should stop
        if (shouldStopGameWorldCreation) {
            std::lock_guard<std::mutex> lock(progressMutex);
            latestProgress.progress = 0.0f;
            latestProgress.message = "Game world creation canceled";
            latestProgress.hasUpdate = true;
            isCreatingGameWorld = false;
            newWorld = nullptr; // Clean up if we're stopping
            return;
        }
        
        // Check if game world creation was successful
        if (!newWorld) {
            std::lock_guard<std::mutex> lock(progressMutex);
            latestProgress.progress = 0.0f;
            latestProgress.message = "Failed to create game world";
            latestProgress.hasUpdate = true;
            isCreatingGameWorld = false;
            return;
        }
        
        // Initialize the world (loads tiles into rendering system)
        if (!newWorld->initialize()) {
            std::lock_guard<std::mutex> lock(progressMutex);
            latestProgress.progress = 0.0f;
            latestProgress.message = "Failed to initialize game world";
            latestProgress.hasUpdate = true;
            isCreatingGameWorld = false;
            newWorld = nullptr;
            return;
        }
        
        // Signal completion
        progressTracker->CompletePhase();
        progressTracker->StartPhase("Finalizing");
        
        std::lock_guard<std::mutex> lock(progressMutex);
        latestProgress.progress = 1.0f;
        latestProgress.message = "Game world creation complete!";
        latestProgress.hasUpdate = true;
    }
    catch (const std::exception& e) {
        // Handle any exceptions
        std::lock_guard<std::mutex> lock(progressMutex);
        latestProgress.progress = 0.0f;
        latestProgress.message = std::string("Error: ") + e.what();
        latestProgress.hasUpdate = true;
    }
    catch (...) {
        // Handle unknown exceptions
        std::lock_guard<std::mutex> lock(progressMutex);
        latestProgress.progress = 0.0f;
        latestProgress.message = "Unknown error during game world creation";
        latestProgress.hasUpdate = true;
    }
    
    isCreatingGameWorld = false;
}

void WorldGenScreen::processProgressMessages() {
    // Check if there's an update to process
    bool hasUpdate = false;
    float progress = 0.0f;
    std::string message;
    bool gameWorldComplete = false; // Flag to check if game world creation is complete
    
    {
        std::lock_guard<std::mutex> lock(progressMutex);
        if (latestProgress.hasUpdate) {
            hasUpdate = true;
            progress = latestProgress.progress;
            message = latestProgress.message;
            latestProgress.hasUpdate = false; // Reset the flag
            
            // Check if game world creation is complete
            if (progress == 1.0f && isCreatingGameWorld == false && newWorld != nullptr) {
                gameWorldComplete = true;
            }
        }
    }
    
    // If there's an update, process it
    if (hasUpdate) {
        // Update the UI directly
        worldGenUI->setProgress(progress, message);
        
        // Special handling for world generation completion message
        if (progress == 1.0f && worldGenerated && !isCreatingGameWorld && !gameWorldComplete) {
            // Update the world renderer now that generation is complete
            worldRenderer->SetWorld(world.get());
            worldGenUI->setState(WorldGen::UIState::Viewing);
            
            // Update UI and stars
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            stars->generate(width, height);
            worldGenUI->onResize(width, height);
        }
        
        // Special handling for game world creation completion
        if (gameWorldComplete) {
            std::cout << "Game world creation complete, transitioning to gameplay..." << std::endl;
            
            // Get components we need from the screen manager
            GameState* gameState = screenManager->getGameState();
            
            // Replace the existing world in the screen manager with our new world
            try {
                screenManager->setWorld(std::move(newWorld));
                
                // Let the game state know we're using a custom world
                gameState->set("custoworld", "true");
                gameState->set("world_sample_rate", std::to_string(gameWorldParams.sampleRate));
                gameState->set("world_seed", gameWorldParams.seed);
                
                // Reset OpenGL state before switching to Game screen
                // This is critical to ensure the Game screen gets the correct viewport and rendering state
                int width, height;
                glfwGetWindowSize(window, &width, &height);
                glViewport(0, 0, width, height);  // Reset to full window viewport
                glDisable(GL_DEPTH_TEST);         // Disable depth testing which Game screen doesn't use
                glEnable(GL_BLEND);               // Ensure blending is enabled
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set standard alpha blending
                glLineWidth(1.0f);                // Reset line width to default
                
                std::cout << "================= PRE-SCREEN TRANSITION DIAGNOSTICS ===================" << std::endl;
                std::cout << "Generator world tiles: " << world->GetTiles().size() << std::endl;
                std::cout << "Game world: valid" << std::endl;
                std::cout << "Camera position: " << 
                          (gameWorldParams.camera ? std::to_string(gameWorldParams.camera->getPosition().x) + ", " + 
                                                    std::to_string(gameWorldParams.camera->getPosition().y) + ", " + 
                                                    std::to_string(gameWorldParams.camera->getPosition().z) : "null") << std::endl;
                std::cout << "About to switch to Game screen..." << std::endl;
                std::cout << "====================================================================" << std::endl;
                
                // Switch to gameplay screen
                screenManager->switchScreen(ScreenType::Gameplay);
                std::cout << "Successfully switched to Game screen" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "ERROR: Exception when handling game world completion: " << e.what() << std::endl;
                worldGenUI->setState(WorldGen::UIState::Viewing); // Return to viewing state on error
            }
            catch (...) {
                std::cerr << "ERROR: Unknown exception during game world completion handling" << std::endl;
                worldGenUI->setState(WorldGen::UIState::Viewing); // Return to viewing state on error
            }
        }
    }
}