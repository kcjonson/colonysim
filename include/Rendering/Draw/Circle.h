#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "../../Vertex.h"
#include "BorderPosition.h"

namespace Rendering {
namespace Draw {

class Circle {
public:
    /**
     * Draw a circle centered at the given position
     * @param center Center point of the circle
     * @param radius Radius of the circle
     * @param color RGBA color (0-1 range)
     * @param vertices Vector of vertices to add to
     * @param indices Vector of indices to add to
     * @param borderColor Border color (if borderWidth > 0)
     * @param borderWidth Width of the border (0 for no border)
     * @param borderPosition Position of the border (inside, outside, center)
     * @param segments Number of segments to approximate the circle (default: 32)
     */
    static void draw(
        const glm::vec2& center, 
        float radius, 
        const glm::vec4& color,
        std::vector<Vertex>& vertices,
        std::vector<unsigned int>& indices,
        const glm::vec4& borderColor = glm::vec4(0.0f),
        float borderWidth = 0.0f,
        BorderPosition borderPosition = BorderPosition::Outside,
        int segments = 32
    );
};

} // namespace Draw
} // namespace Rendering