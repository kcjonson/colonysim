#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "../../Vertex.h"
#include "BorderPosition.h"

namespace Rendering {
namespace Draw {

class Rectangle {
public:
    /**
     * Draw a rectangle centered at the given position
     * @param position Center point of the rectangle
     * @param size Width and height of the rectangle
     * @param color RGBA color (0-1 range)
     * @param vertices Vector of vertices to add to
     * @param indices Vector of indices to add to
     * @param borderColor Border color (if borderWidth > 0)
     * @param borderWidth Width of the border (0 for no border)
     * @param borderPosition Position of the border (inside, outside, center)
     * @param cornerRadius Radius of rounded corners (0 for sharp corners)
     */
    static void draw(
        const glm::vec2& position, 
        const glm::vec2& size, 
        const glm::vec4& color,
        std::vector<Vertex>& vertices,
        std::vector<unsigned int>& indices,
        const glm::vec4& borderColor = glm::vec4(0.0f),
        float borderWidth = 0.0f,
        BorderPosition borderPosition = BorderPosition::Outside,
        float cornerRadius = 0.0f
    );
};

} // namespace Draw
} // namespace Rendering