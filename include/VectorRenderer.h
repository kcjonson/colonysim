#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Vertex.h"
#include "Shader.h"

class VectorRenderer {
public:
    VectorRenderer();
    ~VectorRenderer();

    bool initialize();
    void render(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
                const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model,
                float thickness = 1.0f);
    void cleanup();

private:
    unsigned int VAO, VBO, EBO;
    std::unique_ptr<Shader> shader;
    
    void setupBuffers();
    void setupShader();
}; 