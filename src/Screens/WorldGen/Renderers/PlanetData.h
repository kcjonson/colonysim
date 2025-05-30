#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace WorldGen {

class PlanetData {
public:
    PlanetData(float radius = 1.0f, int resolution = 32);
    
    // Getters
    float getRadius() const { return m_radius; }
    int getResolution() const { return m_resolution; }
    const std::vector<float>& getVertices() const { return m_vertices; }
    const std::vector<unsigned int>& getIndices() const { return m_indices; }
    const std::vector<float>& getTexCoords() const { return m_texCoords; }
    const std::vector<float>& getNormals() const { return m_normals; }
    std::vector<glm::vec3> getVerticesVec3() const;
    
    // Generation
    void generateSphere();
    
private:
    float m_radius;
    int m_resolution;
    
    // Vertex data
    std::vector<float> m_vertices;    // x,y,z coordinates
    std::vector<unsigned int> m_indices;
    std::vector<float> m_texCoords;   // u,v coordinates
    std::vector<float> m_normals;     // nx,ny,nz coordinates
    
    // Helper methods
    void generateVertices();
    void generateIndices();
    void generateTexCoords();
    void generateNormals();
};

} // namespace WorldGen