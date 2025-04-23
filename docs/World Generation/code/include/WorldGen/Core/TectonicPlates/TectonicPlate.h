#pragma once

/**
 * @file TectonicPlate.h
 * @brief Tectonic plate simulation for realistic terrain generation
 */

#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace WorldGen {
namespace Core {

/**
 * @brief Types of tectonic plates
 */
enum class PlateType {
    Continental,  ///< Continental plates (less dense, higher elevation)
    Oceanic       ///< Oceanic plates (more dense, lower elevation)
};

/**
 * @brief Types of plate boundaries
 */
enum class BoundaryType {
    Divergent,    ///< Plates moving away from each other (creates rifts, mid-ocean ridges)
    Convergent,   ///< Plates moving toward each other (creates mountains, trenches)
    Transform     ///< Plates sliding past each other (creates fault lines)
};

/**
 * @brief Represents a boundary between two tectonic plates
 */
struct PlateBoundary {
    int plate1Index;           ///< Index of first plate
    int plate2Index;           ///< Index of second plate
    BoundaryType type;         ///< Type of boundary
    std::vector<glm::vec3> points;  ///< Points defining the boundary
    float stress;              ///< Accumulated stress at boundary
};

/**
 * @brief Represents a single tectonic plate
 */
class TectonicPlate {
public:
    /**
     * @brief Constructor
     * @param id Unique identifier for the plate
     * @param type Type of plate (continental or oceanic)
     * @param center Center point of the plate on the sphere
     */
    TectonicPlate(int id, PlateType type, const glm::vec3& center);
    
    /**
     * @brief Get plate ID
     * @return Unique plate identifier
     */
    int GetId() const { return m_id; }
    
    /**
     * @brief Get plate type
     * @return Type of plate (continental or oceanic)
     */
    PlateType GetType() const { return m_type; }
    
    /**
     * @brief Get plate center
     * @return Center point of the plate on the sphere
     */
    const glm::vec3& GetCenter() const { return m_center; }
    
    /**
     * @brief Get movement vector
     * @return Direction and speed of plate movement
     */
    const glm::vec3& GetMovementVector() const { return m_movementVector; }
    
    /**
     * @brief Set movement vector
     * @param vector Direction and speed of plate movement
     */
    void SetMovementVector(const glm::vec3& vector) { m_movementVector = vector; }
    
    /**
     * @brief Get rotation rate
     * @return Rotation rate around plate center
     */
    float GetRotationRate() const { return m_rotationRate; }
    
    /**
     * @brief Set rotation rate
     * @param rate Rotation rate around plate center
     */
    void SetRotationRate(float rate) { m_rotationRate = rate; }
    
    /**
     * @brief Get tile indices
     * @return Vector of indices of tiles belonging to this plate
     */
    const std::vector<int>& GetTileIndices() const { return m_tileIndices; }
    
    /**
     * @brief Add tile to plate
     * @param tileIndex Index of tile to add
     */
    void AddTile(int tileIndex) { m_tileIndices.push_back(tileIndex); }
    
    /**
     * @brief Get boundaries
     * @return Vector of boundary segments with other plates
     */
    const std::vector<PlateBoundary>& GetBoundaries() const { return m_boundaries; }
    
    /**
     * @brief Add boundary
     * @param boundary Boundary segment to add
     */
    void AddBoundary(const PlateBoundary& boundary) { m_boundaries.push_back(boundary); }
    
    /**
     * @brief Calculate movement at a specific point
     * @param position Point to calculate movement for
     * @return Movement vector at the specified point
     */
    glm::vec3 CalculateMovementAt(const glm::vec3& position) const;
    
    /**
     * @brief Calculate elevation modifier based on plate properties
     * @return Base elevation modifier for this plate type
     */
    float GetBaseElevation() const;

private:
    int m_id;                              ///< Unique identifier
    PlateType m_type;                      ///< Continental or oceanic
    std::vector<int> m_tileIndices;        ///< Indices of tiles belonging to this plate
    glm::vec3 m_movementVector;            ///< Direction and speed of movement
    float m_rotationRate;                  ///< Rotation around plate center
    glm::vec3 m_center;                    ///< Center point of the plate
    std::vector<PlateBoundary> m_boundaries;  ///< Shared boundaries with other plates
};

} // namespace Core
} // namespace WorldGen
