#include "Tile.h"
#include <algorithm>

namespace WorldGen {
namespace Generators {

Tile::Tile(const glm::vec3& center, TileShape shape)
    : center(center)
    , shape(shape)
{
    // Center should always be normalized
    this->center = glm::normalize(this->center);
}

void Tile::AddNeighbor(int neighborIndex)
{
    // Avoid duplicates
    if (std::find(neighbors.begin(), neighbors.end(), neighborIndex) == neighbors.end()) {
        neighbors.push_back(neighborIndex);
    }
}

void Tile::AddVertex(const glm::vec3& vertex)
{
    vertices.push_back(glm::normalize(vertex)); // Ensure vertex is normalized
}

void Tile::SetVertices(const std::vector<glm::vec3>& vertices)
{
    this->vertices = vertices;
    
    // Ensure all vertices are normalized
    for (auto& vertex : this->vertices) {
        vertex = glm::normalize(vertex);
    }
}

void Tile::SetNeighbors(const std::vector<int>& neighbors)
{
    this->neighbors = neighbors;
}

} // namespace Generators
} // namespace WorldGen