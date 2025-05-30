#pragma once

#include <memory>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "PlanetData.h"
#include "../Lithosphere/Plate/TectonicPlate.h"

namespace WorldGen {

class CrustRenderer {
public:
    CrustRenderer();
    ~CrustRenderer();
    
    bool initialize();
    void render(const std::vector<std::shared_ptr<TectonicPlate>>& plates,
                const std::vector<glm::vec3>& planetVertices,
                const glm::mat4& modelMatrix,
                const glm::mat4& viewMatrix,
                const glm::mat4& projectionMatrix);
    void resize(int width, int height);
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // Call this when plates are modified to force geometry regeneration
    void markGeometryDirty() { m_geometryCacheDirty = true; }
    
private:
    // OpenGL objects
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ibo;
    GLuint m_shaderProgram;
    
    // Rendering state
    bool m_enabled = true;
    int m_viewportWidth;
    int m_viewportHeight;
    
    // Cached geometry
    std::vector<float> m_vertexData;      // Combined position, elevation, normal data
    std::vector<unsigned int> m_indices;
    bool m_geometryCacheDirty = true;
    
    // Shader uniforms
    GLint m_modelLoc;
    GLint m_viewLoc;
    GLint m_projectionLoc;
    GLint m_lightDirLoc;
    GLint m_lightColorLoc;
    
    // Helper methods
    bool compileShaders();
    void setupBuffers();
    void updateGeometryCache(const std::vector<std::shared_ptr<TectonicPlate>>& plates,
                             const std::vector<glm::vec3>& planetVertices);
    
    // Helper functions for terrain generation
    float calculateElevationAtVertex(const std::vector<std::shared_ptr<TectonicPlate>>& plates,
                                    int vertexIndex, const glm::vec3& vertexPos);
    glm::vec3 calculateVertexColor(float elevation, const TectonicPlate* plate);
};

} // namespace WorldGen