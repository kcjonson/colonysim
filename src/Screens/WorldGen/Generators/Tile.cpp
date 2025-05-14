#include "Tile.h"
#include <algorithm>

namespace WorldGen {
namespace Generators {

Tile::Tile(const glm::vec3& center, TileType type)
    : m_center(center)
    , m_type(type)
{
    // Center should always be normalized
    m_center = glm::normalize(m_center);
}

void Tile::AddNeighbor(int neighborIndex)
{
    // Avoid duplicates
    if (std::find(m_neighbors.begin(), m_neighbors.end(), neighborIndex) == m_neighbors.end()) {
        m_neighbors.push_back(neighborIndex);
    }
}

void Tile::AddVertex(const glm::vec3& vertex)
{
    m_vertices.push_back(glm::normalize(vertex)); // Ensure vertex is normalized
}

void Tile::SetVertices(const std::vector<glm::vec3>& vertices)
{
    m_vertices = vertices;
    
    // Ensure all vertices are normalized
    for (auto& vertex : m_vertices) {
        vertex = glm::normalize(vertex);
    }
}

void Tile::SetNeighbors(const std::vector<int>& neighbors)
{
    m_neighbors = neighbors;
}

} // namespace Generators
} // namespace WorldGen