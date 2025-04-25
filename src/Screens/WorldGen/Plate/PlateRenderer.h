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
    GLuint m_vbo; // Vertex positions
    GLuint m_colorVbo; // Added: Vertex colors
    GLuint m_shaderProgram;
    
    // Shader uniforms
    GLint m_modelLoc;
    GLint m_viewLoc;
    GLint m_projectionLoc;
    
    // Cached geometry for crust thickness lines
    std::vector<glm::vec3> m_thicknessLineVertices;
    std::vector<glm::vec4> m_thicknessLineColors;
    bool m_thicknessCacheDirty = true;
    size_t m_lastPlateHash = 0;
    void updateThicknessLineCache(const std::vector<std::shared_ptr<TectonicPlate>>& plates,
                                  const std::vector<glm::vec3>& planetVertices);

    // Helper methods
    bool compileShaders();
    void setupBuffers();
};

} // namespace WorldGen