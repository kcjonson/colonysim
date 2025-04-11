#include "Renderer.h"
#include <iostream>

Renderer::Renderer() 
    : projectionMatrix(1.0f)
    , viewMatrix(1.0f) {
}

bool Renderer::initialize() {
    std::cout << "Initializing unified renderer..." << std::endl;
    
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
    
    std::cout << "Unified renderer initialization complete" << std::endl;
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