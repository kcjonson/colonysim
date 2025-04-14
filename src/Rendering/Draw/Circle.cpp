#include "Rendering/Draw/Circle.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>

// Include the BorderPosition enum
#include "VectorGraphics.h"

namespace Rendering {
namespace Draw {

void Circle::draw(
    const glm::vec2& center, 
    float radius, 
    const glm::vec4& color,
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    const glm::vec4& borderColor,
    float borderWidth,
    BorderPosition borderPosition,
    int segments
) {
    // If there's no border, just draw a normal circle
    if (borderWidth <= 0.0f) {
        size_t startIndex = vertices.size();
        
        // Add center vertex
        vertices.push_back({center, color});
        
        // Add vertices around the circle
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * glm::pi<float>() * i / segments;
            glm::vec2 offset(radius * cos(angle), radius * sin(angle));
            vertices.push_back({center + offset, color});
        }

        // Add indices for triangles
        for (int i = 0; i < segments; ++i) {
            indices.push_back(static_cast<unsigned int>(startIndex));
            indices.push_back(static_cast<unsigned int>(startIndex + i + 1));
            indices.push_back(static_cast<unsigned int>(startIndex + i + 2));
        }

        return;
    }

    // Calculate inner and outer radius based on border position
    float innerRadius = radius;
    float outerRadius = radius;
    
    switch (borderPosition) {
        case BorderPosition::Inside:
            innerRadius -= borderWidth;
            break;
        case BorderPosition::Outside:
            outerRadius += borderWidth;
            break;
        case BorderPosition::Center:
            innerRadius -= borderWidth / 2.0f;
            outerRadius += borderWidth / 2.0f;
            break;
    }

    // Ensure inner radius doesn't go negative
    innerRadius = std::max(innerRadius, 0.0f);

    // STEP 1: Draw the inner circle with fill color first
    if (innerRadius > 0) {
        size_t innerStartIndex = vertices.size();
        
        // Add center vertex for inner circle
        vertices.push_back({center, color});
        
        // Add vertices around the inner circle
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * glm::pi<float>() * i / segments;
            glm::vec2 offset(innerRadius * cos(angle), innerRadius * sin(angle));
            vertices.push_back({center + offset, color});
        }

        // Add indices for inner circle triangles
        for (int i = 0; i < segments; ++i) {
            indices.push_back(static_cast<unsigned int>(innerStartIndex));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + i + 1));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + i + 2));
        }
    }
    
    // STEP 2: Draw just the border ring
    // If innerRadius is 0, we're drawing a solid circle with the border color
    if (innerRadius <= 0) {
        size_t outerStartIndex = vertices.size();
        
        // Add center vertex for outer circle
        vertices.push_back({center, borderColor});
        
        // Add vertices around the outer circle
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * glm::pi<float>() * i / segments;
            glm::vec2 offset(outerRadius * cos(angle), outerRadius * sin(angle));
            vertices.push_back({center + offset, borderColor});
        }

        // Add indices for outer circle triangles
        for (int i = 0; i < segments; ++i) {
            indices.push_back(static_cast<unsigned int>(outerStartIndex));
            indices.push_back(static_cast<unsigned int>(outerStartIndex + i + 1));
            indices.push_back(static_cast<unsigned int>(outerStartIndex + i + 2));
        }
    } else {
        // Draw a ring for the border (requires creating triangles between inner and outer circles)
        for (int i = 0; i < segments; ++i) {
            size_t ringStartIndex = vertices.size();
            
            float angle1 = 2.0f * glm::pi<float>() * i / segments;
            float angle2 = 2.0f * glm::pi<float>() * (i + 1) / segments;
            
            glm::vec2 innerPoint1 = center + glm::vec2(innerRadius * cos(angle1), innerRadius * sin(angle1));
            glm::vec2 innerPoint2 = center + glm::vec2(innerRadius * cos(angle2), innerRadius * sin(angle2));
            glm::vec2 outerPoint1 = center + glm::vec2(outerRadius * cos(angle1), outerRadius * sin(angle1));
            glm::vec2 outerPoint2 = center + glm::vec2(outerRadius * cos(angle2), outerRadius * sin(angle2));
            
            // Add vertices for this segment of the ring
            vertices.push_back({innerPoint1, borderColor});
            vertices.push_back({innerPoint2, borderColor});
            vertices.push_back({outerPoint2, borderColor});
            vertices.push_back({outerPoint1, borderColor});
            
            // Add indices for the two triangles making up this segment of the ring
            indices.push_back(static_cast<unsigned int>(ringStartIndex));
            indices.push_back(static_cast<unsigned int>(ringStartIndex + 1));
            indices.push_back(static_cast<unsigned int>(ringStartIndex + 2));
            indices.push_back(static_cast<unsigned int>(ringStartIndex));
            indices.push_back(static_cast<unsigned int>(ringStartIndex + 2));
            indices.push_back(static_cast<unsigned int>(ringStartIndex + 3));
        }
    }
}

} // namespace Draw
} // namespace Rendering