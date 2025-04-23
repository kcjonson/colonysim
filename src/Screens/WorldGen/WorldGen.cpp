#include "WorldGen.h"
#include <iostream>
#include "../ScreenManager.h"
#include "../Game/World.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>
#include "TerrainGenerator.h"
#include "VectorGraphics.h"

// Update constructor definition to accept Camera* and GLFWwindow*
WorldGenScreen::WorldGenScreen(Camera* camera, GLFWwindow* window)
    : lastCursorX(0.0f)
    , lastCursorY(0.0f)
    , sidebarWidth(300.0f)
    , worldWidth(256)
    , worldHeight(256)
    , waterLevel(0.4f)
    , worldGenerated(false)
    , m_cameraDistance(5.0f)
    , m_rotationAngle(0.0f)
    , m_isDragging(false) {
    
    // Generate a random seed
    std::random_device rd;
    seed = rd();
    
    // Create layers with different z-indices and pass pointers
    // Order from back to front:
    starLayer = std::make_shared<Rendering::Layer>(-100.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    backgroundLayer = std::make_shared<Rendering::Layer>(-50.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    previewLayer = std::make_shared<Rendering::Layer>(50.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    sidebarLayer = std::make_shared<Rendering::Layer>(100.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    controlsLayer = std::make_shared<Rendering::Layer>(150.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    buttonLayer = std::make_shared<Rendering::Layer>(200.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    
    // Initialize globe renderer
    m_globeRenderer = std::make_unique<WorldGen::GlobeRenderer>();
}

WorldGenScreen::~WorldGenScreen() {
}

bool WorldGenScreen::initialize() {
    // Initialize globe renderer
    if (!m_globeRenderer->initialize()) {
        std::cerr << "Failed to initialize globe renderer" << std::endl;
        return false;
    }
    
    // Set up OpenGL blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up scroll callback
    glfwSetWindowUserPointer(screenManager->getWindow(), this);
    glfwSetScrollCallback(screenManager->getWindow(), scrollCallback);
    
    // Define buttons
    buttons.clear();
    
    // Generate World button
    createButton("Generate World", 
                 glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),   // Green
                 glm::vec4(0.3f, 0.7f, 0.4f, 1.0f),   // Lighter green
                 [this]() {
                     generatedTerrainData.clear();
                     unsigned int hashedSeed = WorldGen::TerrainGenerator::getHashedSeed(std::to_string(seed));
                     WorldGen::TerrainGenerator::generateTerrain(
                         generatedTerrainData, worldWidth / 2, hashedSeed);
                     worldGenerated = true;
                     layoutUI();
                 });
    
    // Land button (go to gameplay)
    createButton("Land", 
                 glm::vec4(0.2f, 0.5f, 0.8f, 1.0f),   // Blue
                 glm::vec4(0.3f, 0.6f, 0.9f, 1.0f),   // Lighter blue
                 [this]() {
                     if (!worldGenerated) {
                         generatedTerrainData.clear();
                         unsigned int hashedSeed = WorldGen::TerrainGenerator::getHashedSeed(std::to_string(seed));
                         WorldGen::TerrainGenerator::generateTerrain(
                             generatedTerrainData, worldWidth / 2, hashedSeed);
                         worldGenerated = true;
                     }
                     if (screenManager->getWorld()) {
                         screenManager->getWorld()->setTerrainData(generatedTerrainData);
                     }
                     screenManager->switchScreen(ScreenType::Gameplay);
                 });
    
    // Back button
    createButton("Back", 
                 glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),   // Red
                 glm::vec4(0.9f, 0.3f, 0.3f, 1.0f),   // Lighter red
                 [this]() {
                     screenManager->switchScreen(ScreenType::MainMenu);
                 });
    
    // Set initial UI layout
    layoutUI();
    
    return true;
}

void WorldGenScreen::createButton(const std::string& text, const glm::vec4& color, const glm::vec4& hoverColor, const std::function<void()>& callback) {
    MenuButton button;
    button.text = text;
    button.color = color;
    button.hoverColor = hoverColor;
    button.isHovered = false;
    button.callback = callback;
    
    // The actual position and size will be set in layoutUI()
    buttons.push_back(button);
}

void WorldGenScreen::layoutUI() {
    // Get window size
    GLFWwindow* window = screenManager->getWindow();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Clear all layers
    starLayer->clearItems();
    backgroundLayer->clearItems();
    controlsLayer->clearItems();
    buttonLayer->clearItems();
    previewLayer->clearItems();
    sidebarLayer->clearItems();
    
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
    
    // Create sidebar background
    auto sidebar = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(0.0f, 0.0f),
        glm::vec2(sidebarWidth, static_cast<float>(height)),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.9f)
        }),
        100.0f  // Z-index matching sidebarLayer
    );
    sidebarLayer->addItem(sidebar);
    
    // Create title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        "World Generation",
        glm::vec2(40.0f, 70.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 32.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(titleText);
    
    // Create parameter labels and values
    float labelX = 40.0f;
    float valueX = 200.0f;
    float startY = 150.0f;
    float lineHeight = 30.0f;
    
    // Width parameter
    auto widthLabel = std::make_shared<Rendering::Shapes::Text>(
        "Width:",
        glm::vec2(labelX, startY),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(widthLabel);
    
    auto widthValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(worldWidth),
        glm::vec2(valueX, startY),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(widthValue);
    
    // Height parameter
    auto heightLabel = std::make_shared<Rendering::Shapes::Text>(
        "Height:",
        glm::vec2(labelX, startY + lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(heightLabel);
    
    auto heightValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(worldHeight),
        glm::vec2(valueX, startY + lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(heightValue);
    
    // Water level parameter
    auto waterLabel = std::make_shared<Rendering::Shapes::Text>(
        "Water Level:",
        glm::vec2(labelX, startY + 2 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(waterLabel);
    
    auto waterValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(waterLevel),
        glm::vec2(valueX, startY + 2 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(waterValue);
    
    // Seed parameter
    auto seedLabel = std::make_shared<Rendering::Shapes::Text>(
        "Seed:",
        glm::vec2(labelX, startY + 3 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(seedLabel);
    
    auto seedValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(seed),
        glm::vec2(valueX, startY + 3 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(seedValue);
    
    // Button dimensions
    float buttonWidth = 220.0f;
    float buttonHeight = 50.0f;
    float buttonSpacing = 20.0f;
    float sidebarMargin = 40.0f;
    
    // Place buttons in sidebar
    for (size_t i = 0; i < buttons.size(); i++) {
        buttons[i].position.x = sidebarMargin;
        buttons[i].position.y = height - 150.0f - i * (buttonHeight + buttonSpacing);
        buttons[i].size.x = buttonWidth;
        buttons[i].size.y = buttonHeight;
        
        // Create button background
        buttons[i].background = std::make_shared<Rendering::Shapes::Rectangle>(
            buttons[i].position,
            buttons[i].size,
            Rendering::Styles::Rectangle({
                .color = buttons[i].isHovered ? buttons[i].hoverColor : buttons[i].color,
                .cornerRadius = 5.0f
            }),
            200.0f  // Z-index matching buttonLayer
        );
        buttonLayer->addItem(buttons[i].background);
        
        // Create button text
        float textY = buttons[i].position.y + buttons[i].size.y / 2.0f + 8.0f; // Center text vertically
        buttons[i].label = std::make_shared<Rendering::Shapes::Text>(
            buttons[i].text,
            glm::vec2(buttons[i].position.x + buttons[i].size.x / 2.0f, textY),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 24.0f,
                .horizontalAlign = Rendering::TextAlign::Center,
                .verticalAlign = Rendering::TextAlign::Middle
            }),
            200.0f  // Z-index matching buttonLayer
        );
        buttonLayer->addItem(buttons[i].label);
    }
    
    // Add preview area message
    if (worldGenerated) {
        auto generatedMsg = std::make_shared<Rendering::Shapes::Text>(
            "World Generated",
            glm::vec2(sidebarWidth + 20.0f, height / 2.0f),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 32.0f
            }),
            50.0f  // Z-index matching previewLayer
        );
        previewLayer->addItem(generatedMsg);
        
        auto landMsg = std::make_shared<Rendering::Shapes::Text>(
            "Click 'Land' to begin",
            glm::vec2(sidebarWidth + 20.0f, height / 2.0f + 50.0f),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 24.0f
            }),
            50.0f  // Z-index matching previewLayer
        );
        previewLayer->addItem(landMsg);
    } else {
        auto customizeMsg = std::make_shared<Rendering::Shapes::Text>(
            "Use the controls to customize your world",
            glm::vec2(sidebarWidth + 20.0f, height / 2.0f),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 24.0f
            }),
            50.0f  // Z-index matching previewLayer
        );
        previewLayer->addItem(customizeMsg);
        
        auto generateMsg = std::make_shared<Rendering::Shapes::Text>(
            "Click 'Generate World' to create your world",
            glm::vec2(sidebarWidth + 20.0f, height / 2.0f + 40.0f),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 24.0f
            }),
            50.0f  // Z-index matching previewLayer
        );
        previewLayer->addItem(generateMsg);
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
    GLFWwindow* window = screenManager->getWindow();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    m_projectionMatrix = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(width - sidebarWidth) / height,
        0.1f,
        100.0f
    );
    
    // Update globe renderer
    m_globeRenderer->setRotationAngle(m_rotationAngle);
    m_globeRenderer->setCameraDistance(m_cameraDistance);
    m_globeRenderer->resize(width - static_cast<int>(sidebarWidth), height);
}

void WorldGenScreen::render() {
    // Clear the screen
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Get window size
    GLFWwindow* window = screenManager->getWindow();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Enable blending for all transparent objects
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // First render the full-screen background (stars)
    glViewport(0, 0, width, height);
    glDisable(GL_DEPTH_TEST);
    
    // Collect all layers in a vector
    std::vector<std::shared_ptr<Rendering::Layer>> allLayers = {
        starLayer,
        backgroundLayer,
        previewLayer,
        sidebarLayer,
        controlsLayer,
        buttonLayer
    };
    
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
    
    // Then render the planet in its viewport
    glViewport(static_cast<GLint>(sidebarWidth), 0, width - static_cast<GLint>(sidebarWidth), height);
    glEnable(GL_DEPTH_TEST);
    m_globeRenderer->render(m_viewMatrix, m_projectionMatrix);
}

void WorldGenScreen::handleInput() {
    GLFWwindow* window = screenManager->getWindow();
    
    // Handle mouse input for planet rotation
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
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
    } else {
        m_isDragging = false;
    }
    
    // Handle button clicks
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        for (auto& button : buttons) {
            if (isPointInRect(static_cast<float>(xpos), static_cast<float>(ypos),
                             button.position.x, button.position.y,
                             button.size.x, button.size.y)) {
                button.isHovered = true;
                if (button.callback) {
                    button.callback();
                }
            } else {
                button.isHovered = false;
            }
        }
    }
    
    // Check for ESC key to go back to main menu
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        screenManager->switchScreen(ScreenType::MainMenu);
    }
}

void WorldGenScreen::onResize(int width, int height) {
    // Update viewport for planet rendering
    glViewport(static_cast<GLint>(sidebarWidth), 0, width - static_cast<GLint>(sidebarWidth), height);
    
    // Update UI layout
    layoutUI();
}

bool WorldGenScreen::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

void WorldGenScreen::handleScroll(double xoffset, double yoffset) {
    m_cameraDistance = glm::clamp(m_cameraDistance - static_cast<float>(yoffset) * 0.1f, 2.0f, 10.0f);
}

void WorldGenScreen::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    void* ptr = glfwGetWindowUserPointer(window);
    if (ptr) {
        WorldGenScreen* screen = static_cast<WorldGenScreen*>(ptr);
        screen->handleScroll(xoffset, yoffset);
    }
}