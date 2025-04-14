#include "Rendering/Draw/Polygon.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>

// Include the BorderPosition enum
#include "VectorGraphics.h"

namespace Rendering {
namespace Draw {

void Polygon::draw(
    const std::vector<glm::vec2>& points, 
    const glm::vec4& color,
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    const glm::vec4& borderColor,
    float borderWidth,
    BorderPosition borderPosition
) {
    if (points.size() < 3) return;

    // If there's no border, just draw a normal polygon
    if (borderWidth <= 0.0f) {
        size_t startIndex = vertices.size();
        
        // Add vertices
        for (const auto& point : points) {
            vertices.push_back({point, color});
        }

        // Add indices for triangles using triangle fan
        for (size_t i = 1; i < points.size() - 1; ++i) {
            indices.push_back(static_cast<unsigned int>(startIndex));
            indices.push_back(static_cast<unsigned int>(startIndex + i));
            indices.push_back(static_cast<unsigned int>(startIndex + i + 1));
        }

        return;
    }

    // Calculate the centroid of the polygon
    glm::vec2 centroid(0.0f, 0.0f);
    for (const auto& point : points) {
        centroid += point;
    }
    centroid /= static_cast<float>(points.size());
    
    // Calculate scaling factor based on border position and width
    float innerScaleFactor = 1.0f;
    float outerScaleFactor = 1.0f;
    
    switch (borderPosition) {
        case BorderPosition::Inside:
            innerScaleFactor = std::max(0.0f, 1.0f - (borderWidth / 100.0f)); // Simple approximation
            outerScaleFactor = 1.0f;
            break;
        case BorderPosition::Outside:
            innerScaleFactor = 1.0f;
            outerScaleFactor = 1.0f + (borderWidth / 100.0f); // Simple approximation
            break;
        case BorderPosition::Center:
            innerScaleFactor = std::max(0.0f, 1.0f - (borderWidth / 200.0f)); // Half of inside
            outerScaleFactor = 1.0f + (borderWidth / 200.0f); // Half of outside
            break;
    }
    
    // Calculate scaled-down inner polygon points
    std::vector<glm::vec2> innerPoints;
    for (const auto& point : points) {
        // Scale each point toward the centroid
        glm::vec2 innerPoint = centroid + innerScaleFactor * (point - centroid);
        innerPoints.push_back(innerPoint);
    }
    
    // STEP 1: Draw the inner polygon with fill color
    if (innerScaleFactor > 0.0f) {
        size_t innerStartIndex = vertices.size();
        
        // Add vertices for inner polygon
        for (const auto& point : innerPoints) {
            vertices.push_back({point, color});
        }

        // Add indices for inner polygon
        for (size_t i = 1; i < innerPoints.size() - 1; ++i) {
            indices.push_back(static_cast<unsigned int>(innerStartIndex));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + i));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + i + 1));
        }
    }
    
    // STEP 2: Calculate and draw border
    if (innerScaleFactor <= 0.0f) {
        // If no inner polygon, draw the entire polygon with border color
        size_t outerStartIndex = vertices.size();
        
        std::vector<glm::vec2> outerPoints;
        for (const auto& point : points) {
            // Scale each point away from the centroid for outer border
            glm::vec2 outerPoint = centroid + outerScaleFactor * (point - centroid);
            outerPoints.push_back(outerPoint);
        }
        
        // Add vertices for outer polygon
        for (const auto& point : outerPoints) {
            vertices.push_back({point, borderColor});
        }

        // Add indices for outer polygon
        for (size_t i = 1; i < outerPoints.size() - 1; ++i) {
            indices.push_back(static_cast<unsigned int>(outerStartIndex));
            indices.push_back(static_cast<unsigned int>(outerStartIndex + i));
            indices.push_back(static_cast<unsigned int>(outerStartIndex + i + 1));
        }
    } else {
        // Draw border strips between inner and outer points
        std::vector<glm::vec2> outerPoints;
        for (const auto& point : points) {
            // Scale each point away from the centroid for outer border
            glm::vec2 outerPoint = centroid + outerScaleFactor * (point - centroid);
            outerPoints.push_back(outerPoint);
        }
        
        // Create strips between inner and outer polygons to form the border
        for (size_t i = 0; i < points.size(); ++i) {
            size_t nextI = (i + 1) % points.size();
            
            size_t stripIndex = vertices.size();
            
            // Add vertices for this strip segment (quad)
            vertices.push_back({innerPoints[i], borderColor});
            vertices.push_back({innerPoints[nextI], borderColor});
            vertices.push_back({outerPoints[nextI], borderColor});
            vertices.push_back({outerPoints[i], borderColor});
            
            // Add indices for the two triangles forming this quad
            indices.push_back(static_cast<unsigned int>(stripIndex));
            indices.push_back(static_cast<unsigned int>(stripIndex + 1));
            indices.push_back(static_cast<unsigned int>(stripIndex + 2));
            indices.push_back(static_cast<unsigned int>(stripIndex));
            indices.push_back(static_cast<unsigned int>(stripIndex + 2));
            indices.push_back(static_cast<unsigned int>(stripIndex + 3));
        }
    }
}

} // namespace Draw
} // namespace Rendering