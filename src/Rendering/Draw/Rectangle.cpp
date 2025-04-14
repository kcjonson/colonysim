#include "Rendering/Draw/Rectangle.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>

// Include the BorderPosition enum
// We need to include this here since we only forward-declared it in the header
#include "VectorGraphics.h"

namespace Rendering {
namespace Draw {

void Rectangle::draw(
    const glm::vec2& position, 
    const glm::vec2& size, 
    const glm::vec4& color,
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    const glm::vec4& borderColor,
    float borderWidth,
    BorderPosition borderPosition,
    float cornerRadius
) {
    // Limit corner radius to half the minimum dimension
    cornerRadius = std::min(cornerRadius, std::min(size.x, size.y) / 2.0f);
    
    // If corner radius is 0, draw a rectangle without rounded corners
    if (cornerRadius <= 0.0f) {
        // If there's no border, just draw a normal rectangle
        if (borderWidth <= 0.0f) {
            size_t startIndex = vertices.size();
            
            // Add vertices
            vertices.push_back({position + glm::vec2(-size.x/2, -size.y/2), color});
            vertices.push_back({position + glm::vec2(size.x/2, -size.y/2), color});
            vertices.push_back({position + glm::vec2(size.x/2, size.y/2), color});
            vertices.push_back({position + glm::vec2(-size.x/2, size.y/2), color});

            // Add indices
            indices.push_back(static_cast<unsigned int>(startIndex));
            indices.push_back(static_cast<unsigned int>(startIndex + 1));
            indices.push_back(static_cast<unsigned int>(startIndex + 2));
            indices.push_back(static_cast<unsigned int>(startIndex));
            indices.push_back(static_cast<unsigned int>(startIndex + 2));
            indices.push_back(static_cast<unsigned int>(startIndex + 3));

            return;
        }

        // Calculate inner and outer rectangles based on border position
        glm::vec2 innerSize = size;
        glm::vec2 outerSize = size;
        
        switch (borderPosition) {
            case BorderPosition::Inside:
                innerSize -= glm::vec2(borderWidth * 2.0f);
                break;
            case BorderPosition::Outside:
                outerSize += glm::vec2(borderWidth * 2.0f);
                break;
            case BorderPosition::Center:
                innerSize -= glm::vec2(borderWidth);
                outerSize += glm::vec2(borderWidth);
                break;
        }

        // Ensure inner rectangle doesn't have negative dimensions
        innerSize = glm::max(innerSize, glm::vec2(0.0f));

        // STEP 1: Draw the inner fill rectangle first
        if (innerSize.x > 0 && innerSize.y > 0) {
            size_t innerStartIndex = vertices.size();
            
            // Add vertices for inner rectangle
            vertices.push_back({position + glm::vec2(-innerSize.x/2, -innerSize.y/2), color});
            vertices.push_back({position + glm::vec2(innerSize.x/2, -innerSize.y/2), color});
            vertices.push_back({position + glm::vec2(innerSize.x/2, innerSize.y/2), color});
            vertices.push_back({position + glm::vec2(-innerSize.x/2, innerSize.y/2), color});

            // Add indices for inner rectangle
            indices.push_back(static_cast<unsigned int>(innerStartIndex));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + 1));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + 2));
            indices.push_back(static_cast<unsigned int>(innerStartIndex));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + 2));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + 3));
        }
        
        // STEP 2: Now draw the border 
        // To draw just the border (not the entire outer rectangle), we need to create a border shape
        // The simplest way is to use two triangles per side (8 triangles total)
        
        // Calculate the outer and inner corner positions
        glm::vec2 outerTopLeft = position + glm::vec2(-outerSize.x/2, -outerSize.y/2);
        glm::vec2 outerTopRight = position + glm::vec2(outerSize.x/2, -outerSize.y/2);
        glm::vec2 outerBottomRight = position + glm::vec2(outerSize.x/2, outerSize.y/2);
        glm::vec2 outerBottomLeft = position + glm::vec2(-outerSize.x/2, outerSize.y/2);
        
        glm::vec2 innerTopLeft = position + glm::vec2(-innerSize.x/2, -innerSize.y/2);
        glm::vec2 innerTopRight = position + glm::vec2(innerSize.x/2, -innerSize.y/2);
        glm::vec2 innerBottomRight = position + glm::vec2(innerSize.x/2, innerSize.y/2);
        glm::vec2 innerBottomLeft = position + glm::vec2(-innerSize.x/2, innerSize.y/2);
        
        // If inner size is zero, draw a solid rectangle with border color
        if (innerSize.x <= 0 || innerSize.y <= 0) {
            size_t startIndex = vertices.size();
            
            // Add vertices for outer rectangle
            vertices.push_back({outerTopLeft, borderColor});
            vertices.push_back({outerTopRight, borderColor});
            vertices.push_back({outerBottomRight, borderColor});
            vertices.push_back({outerBottomLeft, borderColor});
            
            // Add indices
            indices.push_back(static_cast<unsigned int>(startIndex));
            indices.push_back(static_cast<unsigned int>(startIndex + 1));
            indices.push_back(static_cast<unsigned int>(startIndex + 2));
            indices.push_back(static_cast<unsigned int>(startIndex));
            indices.push_back(static_cast<unsigned int>(startIndex + 2));
            indices.push_back(static_cast<unsigned int>(startIndex + 3));
        } else {
            // Top border
            size_t topBorderIndex = vertices.size();
            vertices.push_back({outerTopLeft, borderColor});
            vertices.push_back({outerTopRight, borderColor});
            vertices.push_back({innerTopRight, borderColor});
            vertices.push_back({innerTopLeft, borderColor});
            
            indices.push_back(static_cast<unsigned int>(topBorderIndex));
            indices.push_back(static_cast<unsigned int>(topBorderIndex + 1));
            indices.push_back(static_cast<unsigned int>(topBorderIndex + 2));
            indices.push_back(static_cast<unsigned int>(topBorderIndex));
            indices.push_back(static_cast<unsigned int>(topBorderIndex + 2));
            indices.push_back(static_cast<unsigned int>(topBorderIndex + 3));
            
            // Right border
            size_t rightBorderIndex = vertices.size();
            vertices.push_back({outerTopRight, borderColor});
            vertices.push_back({outerBottomRight, borderColor});
            vertices.push_back({innerBottomRight, borderColor});
            vertices.push_back({innerTopRight, borderColor});
            
            indices.push_back(static_cast<unsigned int>(rightBorderIndex));
            indices.push_back(static_cast<unsigned int>(rightBorderIndex + 1));
            indices.push_back(static_cast<unsigned int>(rightBorderIndex + 2));
            indices.push_back(static_cast<unsigned int>(rightBorderIndex));
            indices.push_back(static_cast<unsigned int>(rightBorderIndex + 2));
            indices.push_back(static_cast<unsigned int>(rightBorderIndex + 3));
            
            // Bottom border
            size_t bottomBorderIndex = vertices.size();
            vertices.push_back({outerBottomRight, borderColor});
            vertices.push_back({outerBottomLeft, borderColor});
            vertices.push_back({innerBottomLeft, borderColor});
            vertices.push_back({innerBottomRight, borderColor});
            
            indices.push_back(static_cast<unsigned int>(bottomBorderIndex));
            indices.push_back(static_cast<unsigned int>(bottomBorderIndex + 1));
            indices.push_back(static_cast<unsigned int>(bottomBorderIndex + 2));
            indices.push_back(static_cast<unsigned int>(bottomBorderIndex));
            indices.push_back(static_cast<unsigned int>(bottomBorderIndex + 2));
            indices.push_back(static_cast<unsigned int>(bottomBorderIndex + 3));
            
            // Left border
            size_t leftBorderIndex = vertices.size();
            vertices.push_back({outerBottomLeft, borderColor});
            vertices.push_back({outerTopLeft, borderColor});
            vertices.push_back({innerTopLeft, borderColor});
            vertices.push_back({innerBottomLeft, borderColor});
            
            indices.push_back(static_cast<unsigned int>(leftBorderIndex));
            indices.push_back(static_cast<unsigned int>(leftBorderIndex + 1));
            indices.push_back(static_cast<unsigned int>(leftBorderIndex + 2));
            indices.push_back(static_cast<unsigned int>(leftBorderIndex));
            indices.push_back(static_cast<unsigned int>(leftBorderIndex + 2));
            indices.push_back(static_cast<unsigned int>(leftBorderIndex + 3));
        }
    } 
    else {
        // Draw a rounded rectangle with the specified corner radius
        const int cornerSegments = 16; // Increased from 8 to 16 segments per corner for smoother appearance
        
        // If there's no border, draw a simple rounded rectangle
        if (borderWidth <= 0.0f) {
            // Calculate corner centers
            glm::vec2 topLeftCenter = position + glm::vec2(-size.x/2 + cornerRadius, -size.y/2 + cornerRadius);
            glm::vec2 topRightCenter = position + glm::vec2(size.x/2 - cornerRadius, -size.y/2 + cornerRadius);
            glm::vec2 bottomRightCenter = position + glm::vec2(size.x/2 - cornerRadius, size.y/2 - cornerRadius);
            glm::vec2 bottomLeftCenter = position + glm::vec2(-size.x/2 + cornerRadius, size.y/2 - cornerRadius);
            
            // Calculate the rectangle center
            glm::vec2 center = position;
            
            // Add the center vertex
            size_t centerIdx = vertices.size();
            vertices.push_back({center, color});
            
            // Add vertices for the straight sections and corners
            std::vector<unsigned int> perimeterIndices;
            
            // Top-left corner
            for (int i = 0; i <= cornerSegments; i++) {
                float angle = glm::pi<float>() + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                glm::vec2 point = topLeftCenter + glm::vec2(cornerRadius * cos(angle), cornerRadius * sin(angle));
                vertices.push_back({point, color});
                perimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
            }
            
            // Top-right corner
            for (int i = 0; i <= cornerSegments; i++) {
                float angle = glm::pi<float>() * 3.0f / 2.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                glm::vec2 point = topRightCenter + glm::vec2(cornerRadius * cos(angle), cornerRadius * sin(angle));
                vertices.push_back({point, color});
                perimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
            }
            
            // Bottom-right corner
            for (int i = 0; i <= cornerSegments; i++) {
                float angle = 0.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                glm::vec2 point = bottomRightCenter + glm::vec2(cornerRadius * cos(angle), cornerRadius * sin(angle));
                vertices.push_back({point, color});
                perimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
            }
            
            // Bottom-left corner
            for (int i = 0; i <= cornerSegments; i++) {
                float angle = glm::pi<float>() / 2.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                glm::vec2 point = bottomLeftCenter + glm::vec2(cornerRadius * cos(angle), cornerRadius * sin(angle));
                vertices.push_back({point, color});
                perimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
            }
            
            // Create triangles connecting the center to the perimeter
            for (size_t i = 0; i < perimeterIndices.size() - 1; i++) {
                indices.push_back(static_cast<unsigned int>(centerIdx));
                indices.push_back(perimeterIndices[i]);
                indices.push_back(perimeterIndices[i + 1]);
            }
            
            // Close the shape by connecting the last and first perimeter points
            indices.push_back(static_cast<unsigned int>(centerIdx));
            indices.push_back(perimeterIndices[perimeterIndices.size() - 1]);
            indices.push_back(perimeterIndices[0]);
        }
        else {
            // Calculate inner and outer rectangles based on border position
            glm::vec2 innerSize = size;
            glm::vec2 outerSize = size;
            float innerCornerRadius = cornerRadius;
            float outerCornerRadius = cornerRadius;
            
            switch (borderPosition) {
                case BorderPosition::Inside:
                    innerSize -= glm::vec2(borderWidth * 2.0f);
                    innerCornerRadius = std::max(0.0f, cornerRadius - borderWidth);
                    break;
                case BorderPosition::Outside:
                    outerSize += glm::vec2(borderWidth * 2.0f);
                    outerCornerRadius = cornerRadius + borderWidth;
                    break;
                case BorderPosition::Center:
                    innerSize -= glm::vec2(borderWidth);
                    outerSize += glm::vec2(borderWidth);
                    innerCornerRadius = std::max(0.0f, cornerRadius - borderWidth / 2.0f);
                    outerCornerRadius = cornerRadius + borderWidth / 2.0f;
                    break;
            }
            
            // Ensure inner rectangle doesn't have negative dimensions
            innerSize = glm::max(innerSize, glm::vec2(0.0f));
            
            // If inner size is too small, just draw a rounded rectangle with border color
            if (innerSize.x <= 0 || innerSize.y <= 0 || innerCornerRadius <= 0) {
                // Calculate corner centers for outer rectangle
                glm::vec2 topLeftCenter = position + glm::vec2(-outerSize.x/2 + outerCornerRadius, -outerSize.y/2 + outerCornerRadius);
                glm::vec2 topRightCenter = position + glm::vec2(outerSize.x/2 - outerCornerRadius, -outerSize.y/2 + outerCornerRadius);
                glm::vec2 bottomRightCenter = position + glm::vec2(outerSize.x/2 - outerCornerRadius, outerSize.y/2 - outerCornerRadius);
                glm::vec2 bottomLeftCenter = position + glm::vec2(-outerSize.x/2 + outerCornerRadius, outerSize.y/2 - outerCornerRadius);
                
                // Add the center vertex
                size_t centerIdx = vertices.size();
                vertices.push_back({position, borderColor});
                
                // Add vertices for the straight sections and corners
                std::vector<unsigned int> perimeterIndices;
                
                // Top-left corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = glm::pi<float>() + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = topLeftCenter + glm::vec2(outerCornerRadius * cos(angle), outerCornerRadius * sin(angle));
                    vertices.push_back({point, borderColor});
                    perimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Top-right corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = glm::pi<float>() * 3.0f / 2.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = topRightCenter + glm::vec2(outerCornerRadius * cos(angle), outerCornerRadius * sin(angle));
                    vertices.push_back({point, borderColor});
                    perimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Bottom-right corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = 0.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = bottomRightCenter + glm::vec2(outerCornerRadius * cos(angle), outerCornerRadius * sin(angle));
                    vertices.push_back({point, borderColor});
                    perimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Bottom-left corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = glm::pi<float>() / 2.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = bottomLeftCenter + glm::vec2(outerCornerRadius * cos(angle), outerCornerRadius * sin(angle));
                    vertices.push_back({point, borderColor});
                    perimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Create triangles connecting the center to the perimeter
                for (size_t i = 0; i < perimeterIndices.size() - 1; i++) {
                    indices.push_back(static_cast<unsigned int>(centerIdx));
                    indices.push_back(perimeterIndices[i]);
                    indices.push_back(perimeterIndices[i + 1]);
                }
                
                // Close the shape by connecting the last and first perimeter points
                indices.push_back(static_cast<unsigned int>(centerIdx));
                indices.push_back(perimeterIndices[perimeterIndices.size() - 1]);
                indices.push_back(perimeterIndices[0]);
            }
            else {
                // Draw both inner (fill) and outer (border) rounded rectangles
                
                // Step 1: Draw inner rectangle with fill color
                // Calculate corner centers for inner rectangle
                glm::vec2 innerTopLeftCenter = position + glm::vec2(-innerSize.x/2 + innerCornerRadius, -innerSize.y/2 + innerCornerRadius);
                glm::vec2 innerTopRightCenter = position + glm::vec2(innerSize.x/2 - innerCornerRadius, -innerSize.y/2 + innerCornerRadius);
                glm::vec2 innerBottomRightCenter = position + glm::vec2(innerSize.x/2 - innerCornerRadius, innerSize.y/2 - innerCornerRadius);
                glm::vec2 innerBottomLeftCenter = position + glm::vec2(-innerSize.x/2 + innerCornerRadius, innerSize.y/2 - innerCornerRadius);
                
                // Add the center vertex for inner rectangle
                size_t innerCenterIdx = vertices.size();
                vertices.push_back({position, color});
                
                // Add vertices for the inner perimeter
                std::vector<unsigned int> innerPerimeterIndices;
                
                // Top-left corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = glm::pi<float>() + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = innerTopLeftCenter + glm::vec2(innerCornerRadius * cos(angle), innerCornerRadius * sin(angle));
                    vertices.push_back({point, color});
                    innerPerimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Top-right corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = glm::pi<float>() * 3.0f / 2.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = innerTopRightCenter + glm::vec2(innerCornerRadius * cos(angle), innerCornerRadius * sin(angle));
                    vertices.push_back({point, color});
                    innerPerimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Bottom-right corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = 0.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = innerBottomRightCenter + glm::vec2(innerCornerRadius * cos(angle), innerCornerRadius * sin(angle));
                    vertices.push_back({point, color});
                    innerPerimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Bottom-left corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = glm::pi<float>() / 2.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = innerBottomLeftCenter + glm::vec2(innerCornerRadius * cos(angle), innerCornerRadius * sin(angle));
                    vertices.push_back({point, color});
                    innerPerimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Create triangles for inner rectangle
                for (size_t i = 0; i < innerPerimeterIndices.size() - 1; i++) {
                    indices.push_back(static_cast<unsigned int>(innerCenterIdx));
                    indices.push_back(innerPerimeterIndices[i]);
                    indices.push_back(innerPerimeterIndices[i + 1]);
                }
                
                // Close the inner shape
                indices.push_back(static_cast<unsigned int>(innerCenterIdx));
                indices.push_back(innerPerimeterIndices[innerPerimeterIndices.size() - 1]);
                indices.push_back(innerPerimeterIndices[0]);
                
                // Step 2: Draw the border by creating triangles between inner and outer perimeters
                // Calculate corner centers for outer rectangle
                glm::vec2 outerTopLeftCenter = position + glm::vec2(-outerSize.x/2 + outerCornerRadius, -outerSize.y/2 + outerCornerRadius);
                glm::vec2 outerTopRightCenter = position + glm::vec2(outerSize.x/2 - outerCornerRadius, -outerSize.y/2 + outerCornerRadius);
                glm::vec2 outerBottomRightCenter = position + glm::vec2(outerSize.x/2 - outerCornerRadius, outerSize.y/2 - outerCornerRadius);
                glm::vec2 outerBottomLeftCenter = position + glm::vec2(-outerSize.x/2 + outerCornerRadius, outerSize.y/2 - outerCornerRadius);
                
                // Generate vertices for outer perimeter
                std::vector<unsigned int> outerPerimeterIndices;
                
                // Top-left corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = glm::pi<float>() + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = outerTopLeftCenter + glm::vec2(outerCornerRadius * cos(angle), outerCornerRadius * sin(angle));
                    vertices.push_back({point, borderColor});
                    outerPerimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Top-right corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = glm::pi<float>() * 3.0f / 2.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = outerTopRightCenter + glm::vec2(outerCornerRadius * cos(angle), outerCornerRadius * sin(angle));
                    vertices.push_back({point, borderColor});
                    outerPerimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Bottom-right corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = 0.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = outerBottomRightCenter + glm::vec2(outerCornerRadius * cos(angle), outerCornerRadius * sin(angle));
                    vertices.push_back({point, borderColor});
                    outerPerimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Bottom-left corner
                for (int i = 0; i <= cornerSegments; i++) {
                    float angle = glm::pi<float>() / 2.0f + (glm::pi<float>() / 2.0f) * i / cornerSegments;
                    glm::vec2 point = outerBottomLeftCenter + glm::vec2(outerCornerRadius * cos(angle), outerCornerRadius * sin(angle));
                    vertices.push_back({point, borderColor});
                    outerPerimeterIndices.push_back(static_cast<unsigned int>(vertices.size() - 1));
                }
                
                // Create triangles for the border by connecting inner and outer perimeters
                // We need to generate quads (two triangles) between corresponding points
                for (size_t i = 0; i < innerPerimeterIndices.size() - 1; i++) {
                    indices.push_back(innerPerimeterIndices[i]);
                    indices.push_back(innerPerimeterIndices[i + 1]);
                    indices.push_back(outerPerimeterIndices[i + 1]);
                    
                    indices.push_back(innerPerimeterIndices[i]);
                    indices.push_back(outerPerimeterIndices[i + 1]);
                    indices.push_back(outerPerimeterIndices[i]);
                }
                
                // Close the border by connecting the last and first points
                indices.push_back(innerPerimeterIndices[innerPerimeterIndices.size() - 1]);
                indices.push_back(innerPerimeterIndices[0]);
                indices.push_back(outerPerimeterIndices[0]);
                
                indices.push_back(innerPerimeterIndices[innerPerimeterIndices.size() - 1]);
                indices.push_back(outerPerimeterIndices[0]);
                indices.push_back(outerPerimeterIndices[outerPerimeterIndices.size() - 1]);
            }
        }
    }
}

} // namespace Draw
} // namespace Rendering