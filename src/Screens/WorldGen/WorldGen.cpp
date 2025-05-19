#include "WorldGen.h"
#include <iostream>
#include "../ScreenManager.h"
#include "../Game/World.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>
#include "Generators/TerrainGenerator.h"
#include "VectorGraphics.h"
#include <algorithm> // Keep algorithm include

// Initialize the static instances map
std::unordered_map<GLFWwindow*, WorldGenScreen*> WorldGenScreen::s_instances;

// Update constructor definition to accept Camera* and GLFWwindow*
WorldGenScreen::WorldGenScreen(Camera* camera, GLFWwindow* window): 
    lastCursorX(0.0f)
    , lastCursorY(0.0f)
    , worldWidth(256)
    , worldHeight(256)
    , waterLevel(0.4f)
    , worldGenerated(false)
    , m_cameraDistance(5.0f)
    , m_rotationAngle(0.0f)
    , m_isDragging(false)
    , m_window(window) {
    
    // Register this instance in the static map
    s_instances[window] = this;    // Generate a random seed
    std::random_device rd;
    seed = rd();
      // Create a central progress tracker with a callback to update the UI
    m_progressTracker = std::make_shared<WorldGen::ProgressTracker>();
    
    // Initialize Stars
    m_stars = std::make_unique<WorldGen::Stars>(camera, window);
    
    // Initialize WorldGenUI
    m_worldGenUI = std::make_unique<WorldGen::WorldGenUI>(camera, window);

    m_planetParams = WorldGen::PlanetParameters();

    //   // Initialize planet parameters
    // m_planetParams.seed = seed;
    // m_planetParams.radius = 1.0f;
    // m_planetParams.resolution = WorldGen::PlanetParameters().resolution;
}

WorldGenScreen::~WorldGenScreen() {
    // Signal thread to stop
    m_shouldStopGeneration = true;
    
    // Wait for thread to finish
    if (m_generationThread.joinable()) {
        m_generationThread.join();
    }
    
    // Remove scroll callback
    if (m_window) {
        glfwSetScrollCallback(m_window, nullptr);
        // Remove this instance from the static map
        s_instances.erase(m_window);
    }
}

bool WorldGenScreen::initialize() {
    // Initialize UI
    if (!m_worldGenUI->initialize()) {
        std::cerr << "Failed to initialize world gen UI" << std::endl;
        return false;
    }
    
    // Set up the progress tracker callback now that UI is initialized
    m_progressTracker->SetCallback([this](float progress, const std::string& message) {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        m_latestProgress.progress = progress;
        m_latestProgress.message = message;
        m_latestProgress.hasUpdate = true;
    });
    
    // Set up OpenGL blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up scroll callback without overriding window user pointer
    glfwSetScrollCallback(m_window, scrollCallback);    // Initialize world object and renderer, but don't generate the world yet
    m_world = std::make_unique<WorldGen::Generators::World>(m_planetParams, m_progressTracker);
    m_worldRenderer = std::make_unique<WorldGen::Renderers::World>();
    m_worldRenderer->SetWorld(m_world.get());

    // Initialize the projection matrix
    int windowWidth, windowHeight;
    glfwGetWindowSize(m_window, &windowWidth, &windowHeight);
    m_projectionMatrix = glm::perspective(glm::radians(45.0f), static_cast<float>(windowWidth) / windowHeight, 0.1f, 100.0f);
      // Initialize the view matrix
    // Adjust camera distance based on world radius (if world is generated)
    m_cameraDistance = 2.5f; // Default camera distance
    m_viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, m_cameraDistance),  // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f),              // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)               // Up vector
    );      // Register event handlers for UI events

    // Generate World button event
    m_worldGenUI->addEventListener(WorldGen::UIEvent::GenerateWorld, [this]() {
        std::cout << "Generate World button clicked" << std::endl;
        // Switch to generating state
        m_worldGenUI->setState(WorldGen::UIState::Generating);
        
        // Update seed from UI
        m_planetParams.seed = seed;
        
        // Reset the progress tracker
        m_progressTracker->Reset();
        
        // Stop any existing generation thread
        if (m_isGenerating) {
            m_shouldStopGeneration = true;
            if (m_generationThread.joinable()) {
                m_generationThread.join();
            }
            m_shouldStopGeneration = false;
        }
        
        // Start a new generation thread
        m_isGenerating = true;
        m_generationThread = std::thread(&WorldGenScreen::worldGenerationThreadFunc, this);
    });
    
    // Land button event
    m_worldGenUI->addEventListener(WorldGen::UIEvent::GoToLand, [this]() {
        if (!worldGenerated) {
            // If world is not generated, log an error message and return early
            std::cerr << "ERROR: World must be generated before proceeding to land view" << std::endl;
            m_worldGenUI->setState(WorldGen::UIState::ParameterSetup);
            return;
        }
          // Convert the icosahedron world to terrain data
        // Use the progress tracker directly
        m_progressTracker->Reset();
        m_progressTracker->AddPhase("Converting", 0.8f);
        m_progressTracker->AddPhase("Finalizing", 0.2f);
        
        m_progressTracker->StartPhase("Converting");
        convertWorldToTerrainData();
        
        m_progressTracker->CompletePhase();
        m_progressTracker->StartPhase("Finalizing");
        
        // Transfer the terrain data to the game world
        if (screenManager->getWorld()) {
            std::cout << "Transferring " << generatedTerrainData.size() << " tiles to world" << std::endl;
            screenManager->getWorld()->setTerrainData(generatedTerrainData);
        } else {
            std::cerr << "ERROR: World is null in screenManager" << std::endl;
            return;
        }
        
        // Reset OpenGL state before switching to Game screen
        // This is critical to ensure the Game screen gets the correct viewport and rendering state
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        glViewport(0, 0, width, height);  // Reset to full window viewport
        glDisable(GL_DEPTH_TEST);         // Disable depth testing which Game screen doesn't use
        glEnable(GL_BLEND);               // Ensure blending is enabled
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set standard alpha blending
        glLineWidth(1.0f);                // Reset line width to default
        
        // Switch to gameplay screen
        screenManager->switchScreen(ScreenType::Gameplay);
    });
    
    // Back button event
    m_worldGenUI->addEventListener(WorldGen::UIEvent::Back, [this]() {
        // Reset OpenGL state before switching screens
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        glViewport(0, 0, width, height);  // Reset to full window viewport
        glDisable(GL_DEPTH_TEST);         // Disable depth testing
        glEnable(GL_BLEND);               // Ensure blending is enabled
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set standard alpha blending
        glLineWidth(1.0f);                // Reset line width to default
        
        screenManager->switchScreen(ScreenType::MainMenu);
    });
    
    // Set initial UI state
    m_worldGenUI->setState(WorldGen::UIState::ParameterSetup);
      // Set initial UI layout
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    m_stars->generate(width, height);
    m_worldGenUI->onResize(width, height);
    
    return true;
}

void WorldGenScreen::update(float deltaTime) {
    // Process any pending progress messages
    processProgressMessages();
    
    // Rest of existing update code...
    // If world is generated, adjust camera distance based on world radius
    if (worldGenerated && m_world) {
        // Set the camera distance based on the world's radius (add a margin for better visibility)
        float worldRadius = m_world->GetRadius();
        m_cameraDistance = worldRadius * 2.5f;
    }

    // Update camera matrices
    m_viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, m_cameraDistance),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // Update model-view matrix with current rotation
    glm::mat4 rotationMatrix = glm::rotate(
        glm::mat4(1.0f),
        m_rotationAngle,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    m_viewMatrix = m_viewMatrix * rotationMatrix;
    
    // Update projection matrix - use full width for proper aspect ratio
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    m_projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(width) / height,
        0.1f,
        100.0f
    );

    m_worldGenUI->update(deltaTime);
}

void WorldGenScreen::render() {
    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Get window size
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    
    // Use the full window viewport for all rendering
    glViewport(0, 0, width, height);
    
    // --- Render stars (background, always first, blending enabled) ---
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_stars->render();    // --- Render the icosahedron world if generated ---

    if (worldGenerated && m_world && m_worldRenderer) {
        // Enable depth testing for 3D rendering
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Only render in the main area (not over sidebar)
        float sidebarWidth = m_worldGenUI->getSidebarWidth();
        int renderWidth = width - static_cast<int>(sidebarWidth);
        int renderHeight = height;
        glViewport(static_cast<int>(sidebarWidth), 0, renderWidth, renderHeight);
        
        // Calculate correct aspect ratio for the viewport
        float aspectRatio = static_cast<float>(renderWidth) / renderHeight;
        
        // Create adjusted projection matrix for this viewport
        glm::mat4 adjustedProjection = glm::perspective(
            glm::radians(45.0f),  // FOV
            aspectRatio,          // Aspect ratio
            0.1f,                 // Near plane
            100.0f                // Far plane
        );
        
        // Render the world with adjusted projection
        m_worldRenderer->Render(m_viewMatrix, adjustedProjection);        // Fully reset OpenGL state for UI rendering
        glViewport(0, 0, width, height);
        glDisable(GL_DEPTH_TEST);
        
        // Make sure blending is properly set up again for UI rendering
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquation(GL_FUNC_ADD);
        
        // Unbind any active shader program
        glUseProgram(0);
    }

    // --- Render UI layers ---
    m_worldGenUI->render();
}

void WorldGenScreen::handleInput(float deltaTime) {
    // Handle planet rotation with mouse drag
    if (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(m_window, &xpos, &ypos);
        
        // Only allow planet rotation if cursor is in the planet view area (right side of screen)
        if (xpos > m_worldGenUI->getSidebarWidth()) {
            if (!m_isDragging) {
                m_isDragging = true;
                lastCursorX = static_cast<float>(xpos);
                lastCursorY = static_cast<float>(ypos);
            } else {
                float deltaX = static_cast<float>(xpos) - lastCursorX;
                m_rotationAngle += deltaX * 0.01f;
                lastCursorX = static_cast<float>(xpos);
                lastCursorY = static_cast<float>(ypos);
            }
        }
    } else {
        m_isDragging = false;
    }
    
    // Check for ESC key to go back to main menu
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        screenManager->switchScreen(ScreenType::MainMenu);
    }

    m_worldGenUI->handleInput(deltaTime);
}

void WorldGenScreen::onResize(int width, int height) {
    // Update UI layout
    m_worldGenUI->onResize(width, height);
    
    // Update projection matrix
    m_projectionMatrix = glm::perspective(
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
    m_cameraDistance -= static_cast<float>(yoffset) * 0.5f;
    
    // Clamp to reasonable limits
    m_cameraDistance = std::max(1.5f, std::min(10.0f, m_cameraDistance));
}

// Static callback that routes scroll events to the right instance
void WorldGenScreen::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    auto it = s_instances.find(window);
    if (it != s_instances.end()) {
        it->second->handleScroll(xoffset, yoffset);
    }
}

// Convert the icosahedron world to terrain data
void WorldGenScreen::convertWorldToTerrainData() {
    if (!m_world) {
        std::cerr << "ERROR: World not initialized for conversion" << std::endl;
        return;
    }
    
    // Clear previous terrain data
    generatedTerrainData.clear();
    
    // Get the tiles from the world
    const auto& tiles = m_world->GetTiles();
    
    // Counters for terrain types
    int waterTileCount = 0;
    int landTileCount = 0;
    int totalTileCount = 0;
    
    // Report starting conversion
    if (m_progressTracker) {
        m_progressTracker->UpdateProgress(0.1f, "Starting terrain conversion...");
    }
    
    // Track progress
    size_t totalTiles = tiles.size();
    
    // Project tiles onto a 2D grid based on spherical coordinates
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        const auto& center = tile.GetCenter();
        
        // Convert 3D position to longitude/latitude
        float longitude = std::atan2(center.z, center.x);
        float latitude = std::asin(center.y / m_world->GetRadius());
        
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
        if (m_progressTracker && i % 1000 == 0) {
            float progress = 0.1f + (static_cast<float>(i) / totalTiles) * 0.8f;
            std::string message = "Converting tiles to terrain data (" + 
                                 std::to_string(i) + " of " + 
                                 std::to_string(totalTiles) + ")";
            m_progressTracker->UpdateProgress(progress, message);
        }
    }
    
    // Signal completion
    if (m_progressTracker) {
        m_progressTracker->UpdateProgress(0.9f, "Analyzing terrain data...");
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
    if (m_progressTracker) {
        m_progressTracker->UpdateProgress(1.0f, "Terrain conversion complete!");
    }
}

void WorldGenScreen::worldGenerationThreadFunc() {
    try {
        // Generate the world in this background thread
        m_world = WorldGen::Generators::Generator::CreateWorld(m_planetParams, m_progressTracker);
        
        // Check if we should stop
        if (m_shouldStopGeneration) {
            m_isGenerating = false;
            return;
        }
        
        // Generation complete
        worldGenerated = true;
        
        // Set final completion progress
        std::lock_guard<std::mutex> lock(m_progressMutex);
        m_latestProgress.progress = 1.0f;
        m_latestProgress.message = "World generation complete!";
        m_latestProgress.hasUpdate = true;
    }
    catch (const std::exception& e) {
        // Handle any exceptions
        std::lock_guard<std::mutex> lock(m_progressMutex);
        m_latestProgress.progress = 0.0f;
        m_latestProgress.message = std::string("Error: ") + e.what();
        m_latestProgress.hasUpdate = true;
    }
    
    m_isGenerating = false;
}

void WorldGenScreen::processProgressMessages() {
    // Check if there's an update to process
    bool hasUpdate = false;
    float progress = 0.0f;
    std::string message;
    
    {
        std::lock_guard<std::mutex> lock(m_progressMutex);
        if (m_latestProgress.hasUpdate) {
            hasUpdate = true;
            progress = m_latestProgress.progress;
            message = m_latestProgress.message;
            m_latestProgress.hasUpdate = false; // Reset the flag
        }
    }
    
    // If there's an update, process it
    if (hasUpdate) {
        // Update the UI directly
        m_worldGenUI->setProgress(progress, message);
        
        // Special handling for completion message
        if (progress == 1.0f && worldGenerated) {
            // Update the world renderer now that generation is complete
            m_worldRenderer->SetWorld(m_world.get());
            m_worldGenUI->setState(WorldGen::UIState::Viewing);
            
            // Update UI and stars
            int width, height;
            glfwGetWindowSize(m_window, &width, &height);
            m_stars->generate(width, height);
            m_worldGenUI->onResize(width, height);
        }
    }
}