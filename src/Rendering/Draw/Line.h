#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "../../Vertex.h"

namespace Rendering {
namespace Draw {

class Line {
public:
    /**
     * Draw a line between two points
     * @param start Starting point of the line
     * @param end Ending point of the line
     * @param color RGBA color (0-1 range)
     * @param vertices Vector of vertices to add to
     * @param indices Vector of indices to add to
     * @param width Width of the line (default: 1.0f)
     */
    static void draw(
        const glm::vec2& start, 
        const glm::vec2& end, 
        const glm::vec4& color,
        std::vector<Vertex>& vertices,
        std::vector<unsigned int>& indices,
        float width = 1.0f
    );
};

} // namespace Draw
} // namespace Rendering