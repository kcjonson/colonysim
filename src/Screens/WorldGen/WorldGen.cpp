#include "WorldGen.h"
#include <iostream>
#include "../ScreenManager.h"
#include "../Game/World.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>
#include "Core/TerrainGenerator.h"
#include "VectorGraphics.h"
#include <algorithm> // Keep algorithm include
#include "Lithosphere/Lithosphere.h" // Updated path to Lithosphere
#include "Renderers/PlanetData.h" // Updated path from Planet to Renderers

// Initialize the static instances map
std::unordered_map<GLFWwindow*, WorldGenScreen*> WorldGenScreen::s_instances;

// Update constructor definition to accept Camera* and GLFWwindow*
WorldGenScreen::WorldGenScreen(Camera* camera, GLFWwindow* window)
    : lastCursorX(0.0f)
    , lastCursorY(0.0f)
    , worldWidth(256)
    , worldHeight(256)
    , waterLevel(0.4f)
    , worldGenerated(false)
    , m_cameraDistance(3.0f)  // Adjusted camera distance for better visibility of the planet
    , m_rotationAngle(0.0f)
    , m_isDragging(false)
    , m_platesGenerated(false) 
    , m_window(window)
    , m_renderGlobe(true)   // Enable base globe rendering by default for fallback
    , m_renderCrust(true)   // Enable crust rendering by default
    , m_renderPlates(false) // Disable plate boundaries rendering
    , m_simulationTimer(0.0f) // Initialize simulation timer
{
    
    // Register this instance in the static map
    s_instances[window] = this;
    
    // Generate a random seed
    std::random_device rd;
    seed = rd();
    
    // Keep only star layer in WorldGenScreen
    starLayer = std::make_shared<Rendering::Layer>(-100.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    
    // Initialize WorldGenUI
    m_worldGenUI = std::make_unique<WorldGen::WorldGenUI>(camera, window);
    
    // Initialize planet renderers
    WorldGen::PlanetParameters params;
    params.numTectonicPlates = 24;  // Hardcode 8 plates for now
    m_plateGenerator = std::make_unique<WorldGen::PlateGenerator>(params, seed);
    m_plateRenderer = std::make_unique<WorldGen::PlateRenderer>();
    m_globeRenderer = std::make_unique<WorldGen::GlobeRenderer>();
    m_crustRenderer = std::make_unique<WorldGen::CrustRenderer>(); // Initialize the CrustRenderer
    
    // Get planet mesh data after initializing GlobeRenderer
    if (m_globeRenderer) {
        const auto* planetData = m_globeRenderer->getPlanetData(); // Use the added getter
        if (planetData) {
            // Use the new method to get vertices as vec3
            m_planetVertices = planetData->getVerticesVec3();
            m_planetIndices = planetData->getIndices(); // Get indices as well
            std::cout << "Loaded planet mesh: " << m_planetVertices.size() << " vertices, " << m_planetIndices.size() << " indices." << std::endl;
        } else {
            std::cerr << "Error: Failed to get PlanetData from GlobeRenderer." << std::endl;
        }
    } else {
         std::cerr << "Error: GlobeRenderer is null after creation." << std::endl;
    }
}

WorldGenScreen::~WorldGenScreen() {
    // Remove scroll callback
    if (m_window) {
        glfwSetScrollCallback(m_window, nullptr);
        // Remove this instance from the static map
        s_instances.erase(m_window);
    }
}

bool WorldGenScreen::initialize() {
    // Initialize globe renderer
    if (!m_globeRenderer->initialize()) {
        std::cerr << "Failed to initialize globe renderer" << std::endl;
        return false;
    }
    
    // Initialize plate renderer
    if (!m_plateRenderer->initialize()) {
        std::cerr << "Failed to initialize plate renderer" << std::endl;
        return false;
    }
    
    // Initialize crust renderer
    if (!m_crustRenderer->initialize()) {
        std::cerr << "Failed to initialize crust renderer" << std::endl;
        return false;
    }

    // Initialize UI
    if (!m_worldGenUI->initialize()) {
        std::cerr << "Failed to initialize world gen UI" << std::endl;
        return false;
    }
    
    // Set up OpenGL blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up scroll callback without overriding window user pointer
    glfwSetScrollCallback(m_window, scrollCallback);
    
    // Register event handlers for UI events
    
    // Generate World button event
    m_worldGenUI->addEventListener(WorldGen::UIEvent::GenerateWorld, [this]() {
        // Switch to generating state
        m_worldGenUI->setState(WorldGen::UIState::Generating);
        m_worldGenUI->updateProgress(0.1f, "Generating tectonic plates...");
        
        // Get Lithosphere instance from PlateGenerator
        WorldGen::Lithosphere* lithosphere = m_plateGenerator->GetLithosphere();
        if (!lithosphere) {
            std::cerr << "Error: Lithosphere instance is null." << std::endl;
            m_worldGenUI->setState(WorldGen::UIState::ParameterSetup); // Go back if error
            return;
        }

        // Generate plates using Lithosphere (needs vertex data)
        lithosphere->CreatePlates(m_planetVertices);
        m_plates = lithosphere->GetPlates(); // Get the generated plates (reference)

        if (!m_plates.empty()) {
            m_worldGenUI->updateProgress(0.3f, "Detecting boundaries..."); // Adjusted progress

            // Detect initial boundaries using Lithosphere (needs vertex and index data)
            lithosphere->DetectBoundaries(m_planetVertices, m_planetIndices);

            // Explicitly mark geometry as dirty when plates are created
            m_crustRenderer->markGeometryDirty();

            m_worldGenUI->updateProgress(0.4f, "Simulating plate movement...");

            // Simulate some movement (placeholder for now, real simulation happens in Update)
            // lithosphere->Update(0.1f, m_planetVertices, m_planetIndices); // Example: Simulate one step
            // For now, just keep the initial state after creation & boundary detection

            m_worldGenUI->updateProgress(0.7f, "Analyzing boundaries...");

            // Analyze boundaries using Lithosphere (placeholder for now)
            lithosphere->AnalyzeBoundaries(m_planetVertices);

            m_platesGenerated = true;
            std::cout << "Generated " << m_plates.size() << " plates and detected initial boundaries." << std::endl;

            m_worldGenUI->updateProgress(1.0f, "World generation complete!");

            // Switch to viewing state after generation is complete
            worldGenerated = true;
            // Disable base globe rendering now that we have the detailed planet
            m_renderGlobe = false;
            
            // Explicitly mark crust geometry as dirty to force regeneration
            m_crustRenderer->markGeometryDirty();
            std::cout << "Globe renderer disabled, crust renderer activated." << std::endl;
            
            m_worldGenUI->setState(WorldGen::UIState::Viewing);
        } else {
            std::cerr << "Failed to generate plates" << std::endl;
            m_worldGenUI->setState(WorldGen::UIState::ParameterSetup);
        }

        // Update UI
        int width, height;
        glfwGetWindowSize(m_window, &width, &height);
        renderStars(width, height);
        m_worldGenUI->layoutUI(width, height, worldWidth, worldHeight, waterLevel, seed, worldGenerated);
    });
    
    // Land button event
    m_worldGenUI->addEventListener(WorldGen::UIEvent::GoToLand, [this]() {
        if (!worldGenerated) {
            // If not generated yet, go through the generation process first
            m_worldGenUI->setState(WorldGen::UIState::Generating);
            m_worldGenUI->updateProgress(0.1f, "Generating terrain data...");
            
            // Clear previous terrain data
            generatedTerrainData.clear();
            
            // Generate new terrain with the current seed
            unsigned int hashedSeed = WorldGen::TerrainGenerator::getHashedSeed(std::to_string(seed));
            
            // Call the TerrainGenerator with the correct terrain data type
            WorldGen::TerrainGenerator::generateTerrain(
                generatedTerrainData, worldWidth / 2, hashedSeed);
            
            std::cout << "Generated " << generatedTerrainData.size() << " terrain tiles" << std::endl;
            
            worldGenerated = true;
            m_worldGenUI->updateProgress(1.0f, "World generation complete!");
        }
        
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
    renderStars(width, height);
    m_worldGenUI->layoutUI(width, height, worldWidth, worldHeight, waterLevel, seed, worldGenerated);
    
    return true;
}

void WorldGenScreen::renderStars(int width, int height) {
    // Clear the star layer
    starLayer->clearItems();
    
    // Create star background
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> disX(0.0f, static_cast<float>(width));
    std::uniform_real_distribution<float> disY(0.0f, static_cast<float>(height));
    std::uniform_real_distribution<float> disSize(1.0f, 3.0f);
    std::uniform_real_distribution<float> disAlpha(0.5f, 1.0f);
    
    for (int i = 0; i < 200; ++i) {
        float x = disX(gen);
        float y = disY(gen);
        float size = disSize(gen);
        float alpha = disAlpha(gen);
        
        auto star = std::make_shared<Rendering::Shapes::Rectangle>(
            glm::vec2(x, y),
            glm::vec2(size, size),
            Rendering::Styles::Rectangle({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, alpha)
            }),
            -100.0f  // Z-index matching starLayer
        );
        starLayer->addItem(star);
    }
}

void WorldGenScreen::update(float deltaTime) {
    // Create a view matrix that ensures the planet is visible
    // Move camera back along Z-axis and look at origin
    m_viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),    // Position: closer to make planet more visible
        glm::vec3(0.0f, 0.0f, 0.0f),    // Look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)     // Up vector
    );
    
    // Update projection matrix with wider field of view for debugging
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    m_projectionMatrix = glm::perspective(
        glm::radians(60.0f),            // Wider FOV (60 degrees instead of 45)
        static_cast<float>(width) / height,
        0.1f,                           // Near plane
        100.0f                          // Far plane
    );
    
    // Print camera parameters for debugging
    std::cout << "Camera: pos=(0,0," << m_cameraDistance << "), FOV=60°" << std::endl;
    
    // Update globe renderer properties
    m_globeRenderer->setRotationAngle(m_rotationAngle);
    m_globeRenderer->setCameraDistance(m_cameraDistance);
    m_globeRenderer->resize(width, height);
    
    // Remaining simulation code...
    if (m_platesGenerated && !m_disableSimulation) {
        // Accumulate time since last simulation update
        m_simulationTimer += deltaTime;
        
        // Only run simulation when the timer exceeds the update interval
        if (m_simulationTimer >= SIMULATION_UPDATE_INTERVAL) {
            WorldGen::Lithosphere* lithosphere = m_plateGenerator->GetLithosphere();
            if (lithosphere) {
                // Use accumulated time as the simulation step
                float simulationTimeStep = m_simulationTimer * 0.5f; // Adjust speed as needed
                
                // Track significant changes with a counter to reduce regeneration frequency
                static int changeCounter = 0;
                static const int CHANGE_THRESHOLD = 3; // Only update every N changes
                
                // Only mark geometry as dirty if plates actually moved or changed
                bool platesChanged = lithosphere->Update(simulationTimeStep, m_planetVertices, m_planetIndices);
                
                if (platesChanged) {
                    changeCounter++;
                    
                    // Only regenerate geometry periodically based on the counter threshold
                    if (changeCounter >= CHANGE_THRESHOLD) {
                        // After first round of significant changes post-generation, disable further simulation
                        // This prevents continuous updates when viewing the initial planet
                        m_disableSimulation = true;
                        
                        m_crustRenderer->markGeometryDirty();
                        std::cout << "Final plate adjustments applied. Simulation stabilized." << std::endl;
                        changeCounter = 0; // Reset counter
                    } else {
                        std::cout << "Minor plate changes detected (" << changeCounter << "/" << CHANGE_THRESHOLD << "), deferring geometry update." << std::endl;
                    }
                }
            } else {
                std::cerr << "Error: Lithosphere instance is null during update." << std::endl;
            }
            
            // Reset the timer
            m_simulationTimer = 0.0f;
        }
    }
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

    // Calculate horizontal offset for planet in world units
    float sidebarWidthPx = m_worldGenUI->getSidebarWidth();
    float fovY = glm::radians(45.0f);
    float aspect = static_cast<float>(width) / height;
    float tanHalfFovY = tan(fovY / 2.0f);
    float viewHeight = 2.0f * m_cameraDistance * tanHalfFovY;
    float viewWidth = viewHeight * aspect;
    float offsetWorldX = (sidebarWidthPx / static_cast<float>(width)) * viewWidth / 2.0f;
    
    // Set the horizontal offset for the globe renderer
    m_globeRenderer->setHorizontalOffset(offsetWorldX);
    
    // Create model matrix for all planet renderers
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(offsetWorldX, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, m_rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

    // --- Render stars (background, always first, blending enabled) ---
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    starLayer->render();

    // --- Render sidebar background (blending enabled, before globe) ---
    // Only render the sidebar/background layer, not all UI layers yet
    auto uiLayers = m_worldGenUI->getAllLayers();
    // Find the sidebar layer (zIndex == 100.0f by convention)
    for (const auto& layer : uiLayers) {
        if (layer->getZIndex() == 100.0f) {
            layer->render();
            break;
        }
    }

    // Check if we're in Generating state - don't render the planet during generation
    bool isGenerating = (m_worldGenUI->getState() == WorldGen::UIState::Generating);
    
    // Only render the planet if we're not in the generating state
    if (!isGenerating) {
        // --- Render planet visualization (3D rendering with depth test) ---
        glEnable(GL_DEPTH_TEST);
        
        // Print debug information about what we're about to render
        if (m_debugRender) {
            std::cout << "Rendering planet - Globe:" << (m_renderGlobe ? "ON" : "OFF") 
                      << ", Crust:" << (m_renderCrust ? "ON" : "OFF") 
                      << ", PlatesGenerated:" << (m_platesGenerated ? "YES" : "NO")
                      << ", NumPlates:" << (m_plates.empty() ? 0 : m_plates.size())
                      << std::endl;
        }

        // ALWAYS draw crust first if we have plates - the key fix is order of operations
        if (m_renderCrust && m_platesGenerated && !m_plates.empty()) {
            // Completely reset OpenGL state for reliable rendering
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS); // Use default depth function
            glDisable(GL_BLEND); // Disable blending for solid crust
            
            std::cout << "Rendering crust..." << std::endl;
            m_crustRenderer->render(m_plates, m_planetVertices, modelMatrix, m_viewMatrix, m_projectionMatrix);
        }
        
        // Only render globe if explicitly enabled AND we don't have a crust
        if (m_renderGlobe && (!m_platesGenerated || m_plates.empty())) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDisable(GL_BLEND);
            m_globeRenderer->render(m_viewMatrix, m_projectionMatrix);
        }
        
        // Plate boundaries always render last and with blending
        if (m_renderPlates && m_platesGenerated && !m_plates.empty()) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL); // Use LEQUAL for lines to appear over terrain
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glLineWidth(2.0f);
            m_plateRenderer->render(m_plates, m_planetVertices, modelMatrix, m_viewMatrix, m_projectionMatrix);
            glLineWidth(1.0f);
        }
    }

    // --- Render remaining UI layers (controls, buttons, preview, etc) ---
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Render all UI layers except the sidebar (already rendered)
    for (const auto& layer : uiLayers) {
        if (layer->getZIndex() != 100.0f) {
            layer->render();
        }
    }
}

void WorldGenScreen::handleInput() {
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
    
    // Handle button clicks - use wasPressed pattern to detect single click events
    static bool wasPressed = false;
    bool isPressed = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    // Get cursor position
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
    
    // Check if mouse is in the UI sidebar area
    if (xpos <= m_worldGenUI->getSidebarWidth()) {
        // Pass mouse input to UI
        m_worldGenUI->handleButtonClicks(
            static_cast<float>(xpos), 
            static_cast<float>(ypos), 
            isPressed, 
            wasPressed
        );
    }
    
    wasPressed = isPressed;
    
    // Check for ESC key to go back to main menu
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        screenManager->switchScreen(ScreenType::MainMenu);
    }
}

void WorldGenScreen::onResize(int width, int height) {
    // Update viewport to use the full window
    glViewport(0, 0, width, height);
    
    // Update stars and UI layout
    renderStars(width, height);
    m_worldGenUI->layoutUI(width, height, worldWidth, worldHeight, waterLevel, seed, worldGenerated);
}

bool WorldGenScreen::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

void WorldGenScreen::handleScroll(double xoffset, double yoffset) {
    m_cameraDistance = glm::clamp(m_cameraDistance - static_cast<float>(yoffset) * 0.1f, 2.0f, 10.0f);
}

void WorldGenScreen::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Find the instance associated with this window
    auto it = s_instances.find(window);
    if (it != s_instances.end()) {
        it->second->handleScroll(xoffset, yoffset);
    }
}