#include "MountainGenerator.h"
#include "../Plate/TectonicPlate.h"
#include <algorithm>
#include <cmath>

namespace WorldGen {

MountainGenerator::MountainGenerator() {
    // Initialize any necessary properties
}

float MountainGenerator::CalculateElevationWithMountains(
    const glm::vec3& point,
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    int plateIndex,
    float baseElevation,
    const std::vector<glm::vec3>& planetVertices)
{
    // Start with the base elevation
    float elevation = baseElevation;
    
    // Calculate distance to nearest boundary
    float boundaryDistance = DistanceToBoundary(point, plates, plateIndex, planetVertices);
    
    // Only apply mountain effects within a certain distance of boundaries
    const float MOUNTAIN_INFLUENCE_RANGE = 0.3f; // Adjust as needed
    if (boundaryDistance < MOUNTAIN_INFLUENCE_RANGE) {
        // Find the plate boundaries
        const auto& boundaries = plates[plateIndex]->GetBoundaries();
        
        // Get maximum stress at nearby boundaries
        float maxStress = 0.0f;
        float convergenceSpeed = 0.0f;
        
        for (const auto& boundary : boundaries) {
            // Simple calculation - can be made more sophisticated
            if (boundary.stress > maxStress) {
                maxStress = boundary.stress;
                convergenceSpeed = boundary.m_convergenceSpeed;
            }
        }
        
        // Calculate mountain height based on stress and convergence speed
        float mountainHeight = CalculateMountainHeight(maxStress, convergenceSpeed);
        
        // Apply mountain height with falloff based on distance
        float falloff = 1.0f - (boundaryDistance / MOUNTAIN_INFLUENCE_RANGE);
        falloff = falloff * falloff; // Quadratic falloff for more natural look
        
        elevation += mountainHeight * falloff;
    }
    
    return elevation;
}

void MountainGenerator::GenerateMountainRanges(
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    const std::vector<glm::vec3>& planetVertices)
{
    // This is a more comprehensive method that would generate complete mountain ranges
    // Rather than just modifying individual points
    // Implementation would depend on specific needs and data structures
    
    // For now, this is a placeholder for future expansion
}

float MountainGenerator::CalculateMountainHeight(float boundaryStress, float convergenceSpeed) {
    // Higher stress and higher convergence speed = higher mountains
    float height = boundaryStress * convergenceSpeed * 2.0f;
    
    // Apply some minimum and maximum constraints
    return std::min(2.0f, std::max(0.2f, height));
}

float MountainGenerator::DistanceToBoundary(
    const glm::vec3& point,
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    int plateIndex,
    const std::vector<glm::vec3>& planetVertices)
{
    // This is a simplified implementation
    // In a real system, you'd use a spatial indexing structure for performance
    
    float minDistance = 1.0f; // Initialize to maximum possible (normalized coordinates)
    
    const auto& currentPlate = plates[plateIndex];
    
    // Check all boundaries of the current plate
    for (const auto& boundary : currentPlate->GetBoundaries()) {
        // Check edges along the boundary
        for (const auto& edgePair : boundary.m_sharedEdgeIndices) {
            int v1 = edgePair.first;
            int v2 = edgePair.second;
            
            // Skip invalid indices
            if (v1 < 0 || v1 >= planetVertices.size() || 
                v2 < 0 || v2 >= planetVertices.size()) {
                continue;
            }
            
            const glm::vec3& p1 = planetVertices[v1];
            const glm::vec3& p2 = planetVertices[v2];
            
            // Calculate distance from point to line segment (p1-p2) on the sphere
            // This is a simplified calculation and might not be perfectly accurate
            // on a spherical surface
            
            glm::vec3 edge = p2 - p1;
            float edgeLength = glm::length(edge);
            if (edgeLength < 0.0001f) continue; // Skip very short edges
            
            glm::vec3 edgeDir = edge / edgeLength;
            glm::vec3 toPoint = point - p1;
            
            float projection = glm::dot(toPoint, edgeDir);
            projection = std::max(0.0f, std::min(edgeLength, projection));
            
            glm::vec3 closestPoint = p1 + edgeDir * projection;
            float distance = glm::length(point - closestPoint);
            
            minDistance = std::min(minDistance, distance);
        }
    }
    
    return minDistance;
}

} // namespace WorldGen