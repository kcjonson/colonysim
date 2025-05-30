/*
 * DEPRECATED - This class-based tectonic plate implementation is no longer used.
 * The functional plate generation system in Generators/TectonicPlates.h/cpp is used instead.
 * This file is kept for reference but should not be used in new code.
 */
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "TectonicPlate.h"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/constants.hpp> // For pi()
#include <algorithm> // For std::find_if
#include <iostream> // For std::cerr

namespace WorldGen {

TectonicPlate::TectonicPlate(int id, PlateType type, const glm::vec3& center)
    : m_id(id)
    , m_type(type)
    , m_center(glm::normalize(center))
    , m_movementVector(0.0f)
    , m_rotationRate(0.0f)
    , m_totalMass(0.0f) // Initialize new member
{
}

glm::vec3 TectonicPlate::CalculateMovementAt(const glm::vec3& position) const {
    // Ensure position is on the unit sphere if it isn't already
    glm::vec3 posOnSphere = glm::normalize(position);

    // 1. Linear Velocity Component (already tangent to the sphere)
    glm::vec3 linearVelocity = m_movementVector;

    // 2. Rotational Velocity Component
    // Angular velocity vector: axis is the plate center, magnitude is rotation rate
    glm::vec3 angularVelocity = glm::normalize(m_center) * m_rotationRate;

    // Rotational velocity = angular_velocity x position_vector
    // The cross product gives a vector tangent to the sphere at 'position'
    glm::vec3 rotationalVelocity = glm::cross(angularVelocity, posOnSphere);

    // Total velocity is the sum of linear and rotational components
    return linearVelocity + rotationalVelocity;
}

float TectonicPlate::GetBaseElevation() const {
    return (m_type == PlateType::Continental) ? 0.0f : -0.5f; // Example values
}

void TectonicPlate::AddVertex(int vertexIndex) {
    m_vertexIndices.push_back(vertexIndex);
    // Initialize default thickness/age if not already set (optional)
    if (m_vertexCrustThickness.find(vertexIndex) == m_vertexCrustThickness.end()) {
        m_vertexCrustThickness[vertexIndex] = (m_type == PlateType::Continental) ? 0.5f : 0.2f; // Default initial
    }
    if (m_vertexCrustAge.find(vertexIndex) == m_vertexCrustAge.end()) {
         m_vertexCrustAge[vertexIndex] = (m_type == PlateType::Continental) ? 100.0f : 1.0f; // Default initial
    }
}

void TectonicPlate::SetVertexCrustThickness(int vertexIndex, float thickness) {
    m_vertexCrustThickness[vertexIndex] = thickness;
}

void TectonicPlate::SetVertexCrustAge(int vertexIndex, float age) {
    m_vertexCrustAge[vertexIndex] = age;
}

// Get crust thickness for a specific vertex
float TectonicPlate::GetVertexCrustThickness(int vertexIndex) const {
    auto it = m_vertexCrustThickness.find(vertexIndex);
    if (it != m_vertexCrustThickness.end()) {
        return it->second;
    }
    // Return a default or signal an error if vertex isn't part of the plate
    // std::cerr << "Warning: Vertex " << vertexIndex << " not found in thickness map for plate " << m_id << std::endl;
    return (m_type == PlateType::Continental) ? 0.5f : 0.2f; // Return default based on type
}

// Get crust age for a specific vertex
float TectonicPlate::GetVertexCrustAge(int vertexIndex) const {
    auto it = m_vertexCrustAge.find(vertexIndex);
    if (it != m_vertexCrustAge.end()) {
        return it->second;
    }
    // Return a default or signal an error
    // std::cerr << "Warning: Vertex " << vertexIndex << " not found in age map for plate " << m_id << std::endl;
    return (m_type == PlateType::Continental) ? 100.0f : 1.0f; // Return default based on type
}

void TectonicPlate::ClearVertices() {
    m_vertexIndices.clear();
    // Optionally clear thickness/age maps if they should reset on re-assignment
    // m_vertexCrustThickness.clear();
    // m_vertexCrustAge.clear();
}

void TectonicPlate::AddBoundary(const PlateBoundary& boundary) {
    // Avoid adding duplicate boundaries (check based on involved plates)
    for (const auto& existingBoundary : m_boundaries) {
        if ((existingBoundary.plate1Index == boundary.plate1Index && existingBoundary.plate2Index == boundary.plate2Index) ||
            (existingBoundary.plate1Index == boundary.plate2Index && existingBoundary.plate2Index == boundary.plate1Index)) {
            return; // Boundary already exists
        }
    }
    m_boundaries.push_back(boundary);
}

void TectonicPlate::UpdateBoundary(int otherPlateId, const PlateBoundary& updatedBoundary) {
     for (auto& boundary : m_boundaries) {
         // Find the boundary involving the other plate ID
         if ((boundary.plate1Index == m_id && boundary.plate2Index == otherPlateId) ||
             (boundary.plate1Index == otherPlateId && boundary.plate2Index == m_id))
         {
             // Update the relevant fields (don't overwrite plate indices)
             boundary.m_sharedVertexIndices = updatedBoundary.m_sharedVertexIndices;
             boundary.m_sharedEdgeIndicesSet = updatedBoundary.m_sharedEdgeIndicesSet; // Copy set
             boundary.m_sharedEdgeIndices = updatedBoundary.m_sharedEdgeIndices; // Copy vector
             boundary.stress = updatedBoundary.stress;
             boundary.type = updatedBoundary.type;
             boundary.m_relativeMovementMagnitude = updatedBoundary.m_relativeMovementMagnitude;
             boundary.m_convergenceSpeed = updatedBoundary.m_convergenceSpeed;
             boundary.m_transformSpeed = updatedBoundary.m_transformSpeed;
             return; // Found and updated
         }
     }
     // If not found, maybe add it? Or log an error. For now, assume it should exist.
     std::cerr << "Warning: Tried to update non-existent boundary between plate " << m_id << " and " << otherPlateId << std::endl;
}

void TectonicPlate::ClearBoundaries() {
    m_boundaries.clear();
}

} // namespace WorldGen