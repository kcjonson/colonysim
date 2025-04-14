#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Vertex.h"
#include "Shader.h"

class VectorRenderer {
public:
    // Delete copy/move constructors and assignment operators
    VectorRenderer(const VectorRenderer&) = delete;
    VectorRenderer& operator=(const VectorRenderer&) = delete;
    VectorRenderer(VectorRenderer&&) = delete;
    VectorRenderer& operator=(VectorRenderer&&) = delete;

    // Static method to access the singleton instance
    static VectorRenderer& getInstance() {
        static VectorRenderer instance;
        return instance;
    }

    bool initialize();
    void render(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
                const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model,
                float thickness = 1.0f);
    void cleanup();

private:
    // Private constructor for singleton
    VectorRenderer();
    ~VectorRenderer();
    
    unsigned int VAO, VBO, EBO;
    std::unique_ptr<Shader> shader;
    
    void setupBuffers();
    void setupShader();
}; 