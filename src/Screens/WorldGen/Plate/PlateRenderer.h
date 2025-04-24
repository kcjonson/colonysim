#pragma once

#include "TectonicPlate.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace WorldGen {

class PlateRenderer {
public:
    PlateRenderer();
    ~PlateRenderer();
    
    bool initialize();
    void render(const std::vector<std::shared_ptr<TectonicPlate>>& plates,
                const std::vector<glm::vec3>& planetVertices, // Added: Need vertex positions
                const glm::mat4& modelMatrix,
                const glm::mat4& viewMatrix,
                const glm::mat4& projectionMatrix);
    void resize(int width, int height);
    
private:
    // OpenGL objects
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_shaderProgram;
    
    // Shader uniforms
    GLint m_modelLoc;
    GLint m_viewLoc;
    GLint m_projectionLoc;
    GLint m_colorLoc;
    
    // Helper methods
    bool compileShaders();
    void setupBuffers();
};

} // namespace WorldGen