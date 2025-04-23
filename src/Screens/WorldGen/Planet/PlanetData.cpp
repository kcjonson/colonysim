#include "PlanetData.h"
#include <cmath>
#include <glm/glm.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace WorldGen {

PlanetData::PlanetData(float radius, int resolution)
    : m_radius(radius)
    , m_resolution(resolution) {
    generateSphere();
}

void PlanetData::generateSphere() {
    generateVertices();
    generateIndices();
    generateTexCoords();
    generateNormals();
}

void PlanetData::generateVertices() {
    m_vertices.clear();
    m_vertices.reserve((m_resolution + 1) * (m_resolution + 1) * 3);
    
    for (int lat = 0; lat <= m_resolution; lat++) {
        float theta = lat * M_PI / m_resolution;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);
        
        for (int lon = 0; lon <= m_resolution; lon++) {
            float phi = lon * 2 * M_PI / m_resolution;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);
            
            float x = m_radius * sinTheta * cosPhi;
            float y = m_radius * sinTheta * sinPhi;
            float z = m_radius * cosTheta;
            
            m_vertices.push_back(x);
            m_vertices.push_back(y);
            m_vertices.push_back(z);
        }
    }
}

void PlanetData::generateIndices() {
    m_indices.clear();
    m_indices.reserve(m_resolution * m_resolution * 6);
    
    for (int lat = 0; lat < m_resolution; lat++) {
        for (int lon = 0; lon < m_resolution; lon++) {
            int first = lat * (m_resolution + 1) + lon;
            int second = first + m_resolution + 1;
            
            m_indices.push_back(first);
            m_indices.push_back(second);
            m_indices.push_back(first + 1);
            
            m_indices.push_back(second);
            m_indices.push_back(second + 1);
            m_indices.push_back(first + 1);
        }
    }
}

void PlanetData::generateTexCoords() {
    m_texCoords.clear();
    m_texCoords.reserve((m_resolution + 1) * (m_resolution + 1) * 2);
    
    for (int lat = 0; lat <= m_resolution; lat++) {
        float v = 1.0f - static_cast<float>(lat) / m_resolution;
        
        for (int lon = 0; lon <= m_resolution; lon++) {
            float u = static_cast<float>(lon) / m_resolution;
            
            m_texCoords.push_back(u);
            m_texCoords.push_back(v);
        }
    }
}

void PlanetData::generateNormals() {
    m_normals.clear();
    m_normals.reserve(m_vertices.size());
    
    for (size_t i = 0; i < m_vertices.size(); i += 3) {
        glm::vec3 normal = glm::normalize(glm::vec3(
            m_vertices[i],
            m_vertices[i + 1],
            m_vertices[i + 2]
        ));
        
        m_normals.push_back(normal.x);
        m_normals.push_back(normal.y);
        m_normals.push_back(normal.z);
    }
}

} // namespace WorldGen 