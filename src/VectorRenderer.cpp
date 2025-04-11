#include "VectorRenderer.h"
#include <glad/glad.h>
#include <iostream>

VectorRenderer::VectorRenderer() : VAO(0), VBO(0), EBO(0), shader(std::make_unique<Shader>()) {}

VectorRenderer::~VectorRenderer() {
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
    if (EBO != 0) glDeleteBuffers(1, &EBO);
}

bool VectorRenderer::initialize() {
    setupBuffers();
    setupShader();
    return true;
}

void VectorRenderer::setupBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Color attribute
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void VectorRenderer::setupShader() {
    if (!shader->loadFromFile("vector.vert", "vector.frag")) {
        std::cerr << "Failed to load vector shaders" << std::endl;
    }
}

void VectorRenderer::render(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices,
                          const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model,
                          float thickness) {
    shader->use();
    
    // Update uniforms
    shader->setUniform("projection", projection);
    shader->setUniform("view", view);
    shader->setUniform("model", model);

    // Update vertex data
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

    // Update index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

    // Draw
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
} 