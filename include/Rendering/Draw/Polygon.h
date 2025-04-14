#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "../../Vertex.h"
#include "BorderPosition.h"

namespace Rendering {
namespace Draw {

class Polygon {
public:
    /**
     * Draw a polygon defined by a list of points
     * @param points List of points defining the polygon vertices
     * @param color RGBA color (0-1 range)
     * @param vertices Vector of vertices to add to
     * @param indices Vector of indices to add to
     * @param borderColor Border color (if borderWidth > 0)
     * @param borderWidth Width of the border (0 for no border)
     * @param borderPosition Position of the border (inside, outside, center)
     */
    static void draw(
        const std::vector<glm::vec2>& points, 
        const glm::vec4& color,
        std::vector<Vertex>& vertices,
        std::vector<unsigned int>& indices,
        const glm::vec4& borderColor = glm::vec4(0.0f),
        float borderWidth = 0.0f,
        BorderPosition borderPosition = BorderPosition::Outside
    );
};

} // namespace Draw
} // namespace Rendering