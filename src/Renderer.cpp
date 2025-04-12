#include "Renderer.h"
#include <iostream>

Renderer::Renderer() 
    : projectionMatrix(1.0f)
    , viewMatrix(1.0f) {
}

bool Renderer::initialize() {
    // Use a static flag to only print the messages once
    static bool hasInitialized = false;
    
    if (!hasInitialized) {
        std::cout << "Initializing Renderer..." << std::endl;
    
        // Initialize vector renderer
        if (!vectorRenderer.initialize()) {
            std::cerr << "Failed to initialize vector renderer" << std::endl;
            return false;
        }
        
        // Initialize font renderer
        if (!fontRenderer.initialize()) {
            std::cerr << "Failed to initialize font renderer" << std::endl;
            return false;
        }
        
        std::cout << "Renderer initialization complete" << std::endl;
        hasInitialized = true;
        return true;
    }
    
    // Silent reinitialization for subsequent calls
    if (!vectorRenderer.initialize() || !fontRenderer.initialize()) {
        return false;
    }
    
    return true;
}

void Renderer::setProjection(const glm::mat4& projection) {
    projectionMatrix = projection;
}

void Renderer::setView(const glm::mat4& view) {
    viewMatrix = view;
}

void Renderer::renderVector(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
                           const glm::mat4& model, float thickness) {
    vectorRenderer.render(vertices, indices, projectionMatrix, viewMatrix, model, thickness);
}

void Renderer::renderText(const std::string& text, const glm::vec2& position, float scale, const glm::vec3& color) {
    // Set the projection matrix on the font renderer
    fontRenderer.setProjectionMatrix(projectionMatrix);
    
    // Now render the text
    fontRenderer.renderText(text, position, scale, color);
} 