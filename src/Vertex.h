#pragma once

#include <glm/glm.hpp>

struct Vertex {
    glm::vec2 position;  // Position in world/screen coordinates
    glm::vec4 color;     // RGBA color (0-1 range)
    
    Vertex(const glm::vec2& pos, const glm::vec4& col) 
        : position(pos), color(col) {}
}; 