#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "VectorRenderer.h"
#include "FontRenderer.h"

// renderer that manages both vector and text rendering with consistent projections
class Renderer {
public:
    Renderer();
    ~Renderer() = default;

    // Initialize both renderers
    bool initialize();

    // Set projection matrix for both renderers
    void setProjection(const glm::mat4& projection);
    
    // Set view matrix
    void setView(const glm::mat4& view);
    
    // Get the current projection matrix
    const glm::mat4& getProjection() const { return projectionMatrix; }
    
    // Get the current view matrix
    const glm::mat4& getView() const { return viewMatrix; }

    // Render vector shapes
    void renderVector(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
                      const glm::mat4& model = glm::mat4(1.0f), float thickness = 1.0f);
    
    // Render text
    void renderText(const std::string& text, const glm::vec2& position, float scale, const glm::vec3& color);
    
private:
    VectorRenderer vectorRenderer;
    FontRenderer fontRenderer;
    
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
}; 