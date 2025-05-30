#pragma once

#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "PlanetData.h" // Updated include path

namespace WorldGen {

class GlobeRenderer {
public:
    GlobeRenderer();
    ~GlobeRenderer();
    
    bool initialize();
    void render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    void setRotationAngle(float angle);
    void setCameraDistance(float distance);
    void setHorizontalOffset(float offset);
    void resize(int width, int height);
    const PlanetData* getPlanetData() const { return m_planetData.get(); }
    
private:
    std::unique_ptr<PlanetData> m_planetData;
    
    // OpenGL objects
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ibo;
    GLuint m_shaderProgram;
    
    // Rendering state
    glm::mat4 m_modelMatrix;
    float m_rotationAngle;
    float m_cameraDistance;
    float m_horizontalOffset = 0.0f;
    int m_viewportWidth;
    int m_viewportHeight;
    
    // Shader uniforms
    GLint m_modelLoc;
    GLint m_viewLoc;
    GLint m_projectionLoc;
    GLint m_lightDirLoc;
    GLint m_lightColorLoc;
    GLint m_planetColorLoc;
    
    // Helper methods
    bool compileShaders();
    void setupBuffers();
    void updateModelMatrix();
};

} // namespace WorldGen