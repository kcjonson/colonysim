#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <utility>
#include <glm/glm.hpp>
#include <set> // Include set for boundary management

namespace WorldGen {

enum class PlateType {
    Oceanic,
    Continental
};

enum class BoundaryType {
    Convergent, // Plates moving towards each other
    Divergent,  // Plates moving away from each other
    Transform   // Plates sliding past each other
};

struct PlateBoundary {
    int plate1Index;
    int plate2Index;
    std::vector<int> m_sharedVertexIndices;
    // Store edges as pairs of indices, ensuring smaller index is first for consistency
    std::set<std::pair<int, int>> m_sharedEdgeIndicesSet; // Use set during detection
    std::vector<std::pair<int, int>> m_sharedEdgeIndices; // Final vector for rendering/analysis
    float stress = 0.0f;
    BoundaryType type = BoundaryType::Transform; // Default type
    float m_relativeMovementMagnitude = 0.0f;
    float m_convergenceSpeed = 0.0f; // Store calculated speeds
    float m_transformSpeed = 0.0f;   // Store calculated speeds
};


class TectonicPlate {
public:
    TectonicPlate(int id, PlateType type, const glm::vec3& center);

    // Getters
    int GetId() const { return m_id; }
    PlateType GetType() const { return m_type; }
    const glm::vec3& GetCenter() const { return m_center; }
    const glm::vec3& GetMovementVector() const { return m_movementVector; }
    float GetRotationRate() const { return m_rotationRate; }
    const std::vector<int>& GetVertexIndices() const { return m_vertexIndices; }
    std::vector<int>& GetVertexIndices() { return m_vertexIndices; } // Non-const version
    const std::vector<PlateBoundary>& GetBoundaries() const { return m_boundaries; }
    std::vector<PlateBoundary>& GetBoundaries() { return m_boundaries; }
    const std::unordered_map<int, float>& GetVertexCrustThickness() const { return m_vertexCrustThickness; }
    std::unordered_map<int, float>& GetVertexCrustThickness() { return m_vertexCrustThickness; } // Non-const version
    const std::unordered_map<int, float>& GetVertexCrustAge() const { return m_vertexCrustAge; }
    std::unordered_map<int, float>& GetVertexCrustAge() { return m_vertexCrustAge; } // Non-const version
    float GetTotalMass() const { return m_totalMass; }
    float GetVertexCrustThickness(int vertexIndex) const; // Added getter
    float GetVertexCrustAge(int vertexIndex) const; // Added getter

    // Setters
    void SetCenter(const glm::vec3& center) { m_center = glm::normalize(center); } // Ensure center is normalized
    void SetMovementVector(const glm::vec3& vector) { m_movementVector = vector; }
    void SetRotationRate(float rate) { m_rotationRate = rate; }
    void SetTotalMass(float mass) { m_totalMass = mass; }

    // Methods
    void AddVertex(int vertexIndex);
    void ClearVertices(); // Added declaration
    void SetVertexCrustThickness(int vertexIndex, float thickness);
    void SetVertexCrustAge(int vertexIndex, float age);
    void AddBoundary(const PlateBoundary& boundary); // Declaration added
    void UpdateBoundary(int otherPlateId, const PlateBoundary& updatedBoundary); // Declaration added
    void ClearBoundaries(); // Added declaration
    glm::vec3 CalculateMovementAt(const glm::vec3& position) const;
    float GetBaseElevation() const;

private:
    int m_id;
    PlateType m_type;
    glm::vec3 m_center; // Should always be normalized
    glm::vec3 m_movementVector; // Tangent linear velocity
    float m_rotationRate; // Angular speed around m_center axis
    std::vector<int> m_vertexIndices;
    std::vector<PlateBoundary> m_boundaries;
    std::unordered_map<int, float> m_vertexCrustThickness;
    std::unordered_map<int, float> m_vertexCrustAge;
    float m_totalMass = 0.0f;
};

} // namespace WorldGen