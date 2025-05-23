#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "CoordinateSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

CoordinateSystem& CoordinateSystem::getInstance() {
    static CoordinateSystem instance;
    return instance;
}

bool CoordinateSystem::initialize(GLFWwindow* glfwWindow) {
    if (!glfwWindow) {
        std::cerr << "CoordinateSystem: Cannot initialize with null window" << std::endl;
        return false;
    }
    
    window = glfwWindow;
    updateDimensions();
    initialized = true;
    
    std::cout << "CoordinateSystem initialized:" << std::endl;
    std::cout << "  Window size: " << dimensions.windowWidth << "x" << dimensions.windowHeight << std::endl;
    std::cout << "  Framebuffer size: " << dimensions.framebufferWidth << "x" << dimensions.framebufferHeight << std::endl;
    std::cout << "  Scale factors: " << dimensions.scaleX << "x" << dimensions.scaleY << std::endl;
    
    return true;
}

void CoordinateSystem::updateWindowSize(int windowWidth, int windowHeight) {
    if (!initialized) {
        std::cerr << "CoordinateSystem: Cannot update size before initialization" << std::endl;
        return;
    }
    
    updateDimensions();
    
    std::cout << "CoordinateSystem window resized:" << std::endl;
    std::cout << "  New window size: " << dimensions.windowWidth << "x" << dimensions.windowHeight << std::endl;
    std::cout << "  New framebuffer size: " << dimensions.framebufferWidth << "x" << dimensions.framebufferHeight << std::endl;
}

void CoordinateSystem::updateDimensions() {
    if (!window) return;
    
    // Get window size in screen coordinates
    glfwGetWindowSize(window, &dimensions.windowWidth, &dimensions.windowHeight);
    
    // Get framebuffer size in pixels
    glfwGetFramebufferSize(window, &dimensions.framebufferWidth, &dimensions.framebufferHeight);
    
    // Calculate DPI scale factors
    dimensions.scaleX = static_cast<float>(dimensions.framebufferWidth) / dimensions.windowWidth;
    dimensions.scaleY = static_cast<float>(dimensions.framebufferHeight) / dimensions.windowHeight;
}

glm::vec2 CoordinateSystem::screenToFramebuffer(const glm::vec2& screenCoords) const {
    return glm::vec2(
        screenCoords.x * dimensions.scaleX,
        screenCoords.y * dimensions.scaleY
    );
}

glm::vec2 CoordinateSystem::framebufferToScreen(const glm::vec2& framebufferCoords) const {
    return glm::vec2(
        framebufferCoords.x / dimensions.scaleX,
        framebufferCoords.y / dimensions.scaleY
    );
}

glm::mat4 CoordinateSystem::createScreenSpaceProjection() const {
    // Screen space projection: (0,0) = top-left, Y increases downward
    // This is good for UI elements that should be positioned from the top-left
    const float width = static_cast<float>(dimensions.windowWidth);
    const float height = static_cast<float>(dimensions.windowHeight);
    
    return glm::ortho(0.0f, width, height, 0.0f, -1000.0f, 1000.0f);
}

glm::mat4 CoordinateSystem::createWorldSpaceProjection() const {
    // World space projection: (0,0) = center, Y increases upward
    // This is good for world content and game objects
    const float halfWidth = dimensions.windowWidth / 2.0f;
    const float halfHeight = dimensions.windowHeight / 2.0f;
    
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1000.0f, 1000.0f);
}

glm::mat4 CoordinateSystem::createCenteredProjection() const {
    // Centered projection: (0,0) = center, Y increases upward
    // Similar to world space but with explicit naming for clarity
    return createWorldSpaceProjection();
}

void CoordinateSystem::setFullViewport() const {
    glViewport(0, 0, dimensions.framebufferWidth, dimensions.framebufferHeight);
}

void CoordinateSystem::setViewport(int x, int y, int width, int height) const {
    // Scale viewport coordinates to framebuffer coordinates
    int scaledX = static_cast<int>(x * dimensions.scaleX);
    int scaledY = static_cast<int>(y * dimensions.scaleY);
    int scaledWidth = static_cast<int>(width * dimensions.scaleX);
    int scaledHeight = static_cast<int>(height * dimensions.scaleY);
    
    glViewport(scaledX, scaledY, scaledWidth, scaledHeight);
}

void CoordinateSystem::setScaledViewport(float x, float y, float width, float height) const {
    // Set viewport using normalized coordinates (0.0 to 1.0)
    int pixelX = static_cast<int>(x * dimensions.framebufferWidth);
    int pixelY = static_cast<int>(y * dimensions.framebufferHeight);
    int pixelWidth = static_cast<int>(width * dimensions.framebufferWidth);
    int pixelHeight = static_cast<int>(height * dimensions.framebufferHeight);
    
    glViewport(pixelX, pixelY, pixelWidth, pixelHeight);
}

float CoordinateSystem::getAspectRatio() const {
    return static_cast<float>(dimensions.windowWidth) / dimensions.windowHeight;
}

glm::vec2 CoordinateSystem::getWindowCenter() const {
    return glm::vec2(dimensions.windowWidth / 2.0f, dimensions.windowHeight / 2.0f);
}

glm::vec2 CoordinateSystem::getWindowSize() const {
    return glm::vec2(static_cast<float>(dimensions.windowWidth), static_cast<float>(dimensions.windowHeight));
}

void CoordinateSystem::resetOpenGLState() const {
    if (!initialized) return;
    
    // Reset to full viewport
    setFullViewport();
    
    // Reset OpenGL state to safe defaults for UI rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glLineWidth(1.0f);
    
    // Unbind any active shader program
    glUseProgram(0);
    
    // Clear any active texture bindings
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    std::cout << "CoordinateSystem: OpenGL state reset to UI defaults" << std::endl;
}

void CoordinateSystem::saveOpenGLState() {
    if (!initialized) return;
    
    // Save current OpenGL state
    savedState.depthTest = glIsEnabled(GL_DEPTH_TEST);
    savedState.blend = glIsEnabled(GL_BLEND);
    
    glGetIntegerv(GL_BLEND_SRC_RGB, &savedState.blendSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &savedState.blendDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &savedState.blendSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &savedState.blendDstAlpha);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &savedState.blendEquationRGB);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &savedState.blendEquationAlpha);
    
    glGetFloatv(GL_LINE_WIDTH, &savedState.lineWidth);
    glGetIntegerv(GL_VIEWPORT, savedState.viewport);
}

void CoordinateSystem::restoreOpenGLState() const {
    if (!initialized) return;
    
    // Restore saved OpenGL state
    if (savedState.depthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    if (savedState.blend) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }
    
    glBlendFuncSeparate(savedState.blendSrcRGB, savedState.blendDstRGB, 
                        savedState.blendSrcAlpha, savedState.blendDstAlpha);
    glBlendEquationSeparate(savedState.blendEquationRGB, savedState.blendEquationAlpha);
    
    glLineWidth(savedState.lineWidth);
    glViewport(savedState.viewport[0], savedState.viewport[1], 
               savedState.viewport[2], savedState.viewport[3]);
}
