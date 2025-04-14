#include "Rendering/Draw/Line.h"
#include <glm/gtc/constants.hpp>

namespace Rendering {
namespace Draw {

void Line::draw(
    const glm::vec2& start, 
    const glm::vec2& end, 
    const glm::vec4& color,
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    float width
) {
    glm::vec2 direction = glm::normalize(end - start);
    glm::vec2 perpendicular(-direction.y, direction.x);
    glm::vec2 offset = perpendicular * (width / 2.0f);

    size_t startIndex = vertices.size();
    
    // Add vertices
    vertices.push_back({start - offset, color});
    vertices.push_back({start + offset, color});
    vertices.push_back({end + offset, color});
    vertices.push_back({end - offset, color});

    // Add indices
    indices.push_back(static_cast<unsigned int>(startIndex));
    indices.push_back(static_cast<unsigned int>(startIndex + 1));
    indices.push_back(static_cast<unsigned int>(startIndex + 2));
    indices.push_back(static_cast<unsigned int>(startIndex));
    indices.push_back(static_cast<unsigned int>(startIndex + 2));
    indices.push_back(static_cast<unsigned int>(startIndex + 3));
}

} // namespace Draw
} // namespace Rendering