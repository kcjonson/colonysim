#include "Renderer.h"
#include <GLFW/glfw3.h>
#include <iostream>

Renderer::Renderer(int width, int height, const std::string& title) 
    : window(nullptr), camera(nullptr), entityManager(nullptr), inputManager(nullptr) {
    // Initialize GLFW
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // Set up viewport
    glViewport(0, 0, width, height);

    // Create camera, entity manager, and input manager
    camera = std::make_unique<Camera>();
    entityManager = std::make_unique<EntityManager>();
    inputManager = std::make_unique<InputManager>(window, *camera, *entityManager);
}

Renderer::~Renderer() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void Renderer::initialize() {
    // Set up OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::render() {
    // Clear the screen
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Update input
    inputManager->update(0.016f); // Assuming 60 FPS

    // Get view and projection matrices
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = camera->getProjectionMatrix();

    // Render vector graphics
    vectorGraphics.render(view, projection);

    // Swap buffers and poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Renderer::setWindowSize(int width, int height) {
    glViewport(0, 0, width, height);
    camera->setOrthographicProjection(-width/2.0f, width/2.0f, -height/2.0f, height/2.0f, -1.0f, 1.0f);
} 