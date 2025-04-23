#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace WorldGen {

enum class PlateType {
    Continental,
    Oceanic
};

enum class BoundaryType {
    Convergent,
    Divergent,
    Transform
};

struct PlateBoundary {
    int plate1Index;
    int plate2Index;
    std::vector<glm::vec3> points;
    float stress;
    BoundaryType type;
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
    const std::vector<int>& GetTileIndices() const { return m_tileIndices; }
    const std::vector<PlateBoundary>& GetBoundaries() const { return m_boundaries; }
    std::vector<PlateBoundary>& GetBoundaries() { return m_boundaries; }
    
    // Setters
    void SetMovementVector(const glm::vec3& vector) { m_movementVector = vector; }
    void SetRotationRate(float rate) { m_rotationRate = rate; }
    
    // Methods
    void AddTile(int tileIndex) { m_tileIndices.push_back(tileIndex); }
    void AddBoundary(const PlateBoundary& boundary) { m_boundaries.push_back(boundary); }
    void UpdateBoundaryStress(int otherPlateId, float stress) {
        for (auto& boundary : m_boundaries) {
            if (boundary.plate2Index == otherPlateId || boundary.plate1Index == otherPlateId) {
                boundary.stress = stress;
            }
        }
    }
    glm::vec3 CalculateMovementAt(const glm::vec3& position) const;
    float GetBaseElevation() const;

private:
    int m_id;
    PlateType m_type;
    glm::vec3 m_center;
    glm::vec3 m_movementVector;
    float m_rotationRate;
    std::vector<int> m_tileIndices;
    std::vector<PlateBoundary> m_boundaries;
};

} // namespace WorldGen 