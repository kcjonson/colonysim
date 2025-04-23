#include "WorldGen.h"
#include <iostream>
#include "../ScreenManager.h"
#include "../Game/World.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>
#include "TerrainGenerator.h"
#include "VectorGraphics.h"
#include <algorithm>

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
    , m_cameraDistance(5.0f)
    , m_rotationAngle(0.0f)
    , m_isDragging(false)
    , m_platesGenerated(false) 
    , m_window(window) {
    
    // Register this instance in the static map
    s_instances[window] = this;
    
    // Generate a random seed
    std::random_device rd;
    seed = rd();
    
    // Keep only star layer in WorldGenScreen
    starLayer = std::make_shared<Rendering::Layer>(-100.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    
    // Initialize WorldGenUI
    m_worldGenUI = std::make_unique<WorldGen::WorldGenUI>(camera, window);
    
    // Initialize plate generator and renderer
    WorldGen::PlanetParameters params;
    params.numTectonicPlates = 8;  // Hardcode 8 plates for now
    m_plateGenerator = std::make_unique<WorldGen::PlateGenerator>(params, seed);
    m_plateRenderer = std::make_unique<WorldGen::PlateRenderer>();
    m_globeRenderer = std::make_unique<WorldGen::GlobeRenderer>();
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
        
        // Generate plates
        m_plates = m_plateGenerator->GeneratePlates();
        if (!m_plates.empty()) {
            m_worldGenUI->updateProgress(0.4f, "Simulating plate movement...");
            
            // Simulate some movement to make boundaries more interesting
            m_plateGenerator->SimulatePlateMovement(m_plates, 10);
            
            m_worldGenUI->updateProgress(0.7f, "Analyzing boundaries...");
            
            // Analyze boundaries to set stress levels
            m_plateGenerator->AnalyzeBoundaries(m_plates);
            
            m_platesGenerated = true;
            std::cout << "Generated " << m_plates.size() << " plates" << std::endl;
            
            m_worldGenUI->updateProgress(1.0f, "World generation complete!");
            
            // Switch to viewing state after generation is complete
            worldGenerated = true;
            m_worldGenUI->setState(WorldGen::UIState::Viewing);
        } else {
            std::cerr << "Failed to generate plates" << std::endl;
            // Return to parameter setup state if generation failed
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
        }
        
        // Switch to gameplay screen
        screenManager->switchScreen(ScreenType::Gameplay);
    });
    
    // Back button event
    m_worldGenUI->addEventListener(WorldGen::UIEvent::Back, [this]() {
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
    // Update camera matrices
    m_viewMatrix = glm::lookAt(
        glm::vec3(0.0f, 0.0f, m_cameraDistance),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // Update projection matrix
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    m_projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(width - m_worldGenUI->getSidebarWidth()) / height,
        0.1f,
        100.0f
    );
    
    // Update globe renderer
    m_globeRenderer->setRotationAngle(m_rotationAngle);
    m_globeRenderer->setCameraDistance(m_cameraDistance);
    m_globeRenderer->resize(width - static_cast<int>(m_worldGenUI->getSidebarWidth()), height);
}

void WorldGenScreen::render() {
    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Get window size
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    
    // Enable blending for all transparent objects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // First render the full-screen background (stars)
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    
    // Get UI layers
    std::vector<std::shared_ptr<Rendering::Layer>> uiLayers = m_worldGenUI->getAllLayers();
    
    // Collect all layers in a vector
    std::vector<std::shared_ptr<Rendering::Layer>> allLayers = {starLayer};
    allLayers.insert(allLayers.end(), uiLayers.begin(), uiLayers.end());
    
    // Sort layers by z-index
    std::sort(allLayers.begin(), allLayers.end(),
        [](const std::shared_ptr<Rendering::Layer>& a, const std::shared_ptr<Rendering::Layer>& b) {
            return a->getZIndex() < b->getZIndex();
        }
    );
    
    // Render layers in sorted order
    for (const auto& layer : allLayers) {
        layer->render();
    }
    
    // Then render the planet and plates in its viewport
    glViewport(static_cast<GLint>(m_worldGenUI->getSidebarWidth()), 0, 
              width - static_cast<GLint>(m_worldGenUI->getSidebarWidth()), height);
    glEnable(GL_DEPTH_TEST);
    
    // Create model matrix for both globe and plates
    glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f), m_rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Render the globe
    m_globeRenderer->render(m_viewMatrix, m_projectionMatrix);
    
    // Render plates if they've been generated
    if (m_platesGenerated && !m_plates.empty()) {
        glLineWidth(2.0f); // Make plate boundaries more visible
        m_plateRenderer->render(m_plates, modelMatrix, m_viewMatrix, m_projectionMatrix);
        glLineWidth(1.0f);
    }
}

void WorldGenScreen::handleInput() {
    // Handle mouse input for planet rotation
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
    // Update viewport for planet rendering
    glViewport(static_cast<GLint>(m_worldGenUI->getSidebarWidth()), 0, 
              width - static_cast<GLint>(m_worldGenUI->getSidebarWidth()), height);
    
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