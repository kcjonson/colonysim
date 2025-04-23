#pragma once

/**
 * @file RiverGenerator.h
 * @brief Simulates water flow to create rivers and lakes
 */

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "WorldGen/PlanetParameters.h"
#include "WorldGen/ProgressTracker.h"

namespace WorldGen {
namespace Core {

/**
 * @brief Represents a river segment
 */
struct RiverSegment {
    glm::vec3 startPoint;     ///< Start point of segment
    glm::vec3 endPoint;       ///< End point of segment
    float width;              ///< Width of river at this segment
    float flow;               ///< Water flow volume
    int nextSegmentIndex;     ///< Index of next downstream segment (-1 if none)
};

/**
 * @brief Represents a complete river
 */
class River {
public:
    /**
     * @brief Constructor
     * @param id Unique identifier for the river
     */
    explicit River(int id);
    
    /**
     * @brief Get river ID
     * @return Unique river identifier
     */
    int GetId() const { return m_id; }
    
    /**
     * @brief Get river segments
     * @return Vector of river segments
     */
    const std::vector<RiverSegment>& GetSegments() const { return m_segments; }
    
    /**
     * @brief Add segment to river
     * @param segment River segment to add
     * @return Index of added segment
     */
    int AddSegment(const RiverSegment& segment);
    
    /**
     * @brief Get total river length
     * @return Total length of river
     */
    float GetTotalLength() const;
    
    /**
     * @brief Get river source point
     * @return Source (highest) point of river
     */
    glm::vec3 GetSource() const;
    
    /**
     * @brief Get river mouth point
     * @return Mouth (lowest) point of river
     */
    glm::vec3 GetMouth() const;
    
    /**
     * @brief Get average flow rate
     * @return Average water flow rate
     */
    float GetAverageFlow() const;

private:
    int m_id;                           ///< Unique identifier
    std::vector<RiverSegment> m_segments;  ///< River segments
};

/**
 * @brief Represents a lake
 */
class Lake {
public:
    /**
     * @brief Constructor
     * @param id Unique identifier for the lake
     * @param center Center point of lake
     */
    Lake(int id, const glm::vec3& center);
    
    /**
     * @brief Get lake ID
     * @return Unique lake identifier
     */
    int GetId() const { return m_id; }
    
    /**
     * @brief Get lake center
     * @return Center point of lake
     */
    const glm::vec3& GetCenter() const { return m_center; }
    
    /**
     * @brief Get lake boundary points
     * @return Vector of points defining lake boundary
     */
    const std::vector<glm::vec3>& GetBoundaryPoints() const { return m_boundaryPoints; }
    
    /**
     * @brief Add boundary point
     * @param point Point on lake boundary
     */
    void AddBoundaryPoint(const glm::vec3& point);
    
    /**
     * @brief Get lake area
     * @return Approximate area of lake
     */
    float GetArea() const;
    
    /**
     * @brief Get lake depth
     * @return Maximum depth of lake
     */
    float GetDepth() const { return m_depth; }
    
    /**
     * @brief Set lake depth
     * @param depth Maximum depth of lake
     */
    void SetDepth(float depth) { m_depth = depth; }
    
    /**
     * @brief Get inflow rivers
     * @return Vector of IDs of rivers flowing into this lake
     */
    const std::vector<int>& GetInflowRivers() const { return m_inflowRivers; }
    
    /**
     * @brief Add inflow river
     * @param riverId ID of river flowing into this lake
     */
    void AddInflowRiver(int riverId);
    
    /**
     * @brief Get outflow river
     * @return ID of river flowing out of this lake (-1 if none)
     */
    int GetOutflowRiver() const { return m_outflowRiver; }
    
    /**
     * @brief Set outflow river
     * @param riverId ID of river flowing out of this lake
     */
    void SetOutflowRiver(int riverId) { m_outflowRiver = riverId; }

private:
    int m_id;                           ///< Unique identifier
    glm::vec3 m_center;                 ///< Center point of lake
    std::vector<glm::vec3> m_boundaryPoints;  ///< Points defining lake boundary
    float m_depth;                      ///< Maximum depth of lake
    std::vector<int> m_inflowRivers;    ///< IDs of rivers flowing into this lake
    int m_outflowRiver;                 ///< ID of river flowing out of this lake (-1 if none)
};

/**
 * @brief Simulates water flow to create rivers and lakes
 * 
 * This class handles the simulation of water flow across terrain,
 * creating realistic river networks and lakes based on precipitation
 * and elevation data.
 */
class RiverGenerator {
public:
    /**
     * @brief Constructor
     * @param parameters Planet parameters
     */
    explicit RiverGenerator(const PlanetParameters& parameters);
    
    /**
     * @brief Destructor
     */
    ~RiverGenerator();
    
    /**
     * @brief Generate rivers and lakes
     * @param elevationData Terrain elevation data
     * @param precipitationData Precipitation data
     * @param resolution Resolution of the grid
     * @param progressTracker Optional progress tracker
     * @return Pair of vectors containing generated rivers and lakes
     */
    std::pair<std::vector<std::shared_ptr<River>>, std::vector<std::shared_ptr<Lake>>> 
    GenerateRiversAndLakes(
        const std::vector<float>& elevationData,
        const std::vector<float>& precipitationData,
        int resolution,
        std::shared_ptr<ProgressTracker> progressTracker = nullptr);
    
    /**
     * @brief Apply erosion to terrain
     * @param elevationData Terrain elevation data to modify
     * @param rivers Vector of rivers
     * @param resolution Resolution of the grid
     * @param progressTracker Optional progress tracker
     */
    void ApplyErosion(
        std::vector<float>& elevationData,
        const std::vector<std::shared_ptr<River>>& rivers,
        int resolution,
        std::shared_ptr<ProgressTracker> progressTracker = nullptr);

private:
    PlanetParameters m_parameters;
    int m_resolution;
    
    // Helper methods
    std::vector<float> CalculateWaterFlow(
        const std::vector<float>& elevationData,
        const std::vector<float>& precipitationData);
    
    std::vector<int> CalculateFlowDirections(
        const std::vector<float>& elevationData);
    
    void TraceRiverPaths(
        const std::vector<float>& flowData,
        const std::vector<int>& flowDirections,
        std::vector<std::shared_ptr<River>>& rivers);
    
    void IdentifyLakes(
        const std::vector<float>& elevationData,
        const std::vector<float>& flowData,
        std::vector<std::shared_ptr<Lake>>& lakes);
    
    void ConnectRiversAndLakes(
        std::vector<std::shared_ptr<River>>& rivers,
        std::vector<std::shared_ptr<Lake>>& lakes);
    
    float CalculateRiverWidth(float flow) const;
    
    // Utility methods
    int GetGridIndex(float latitude, float longitude) const;
    std::vector<int> GetNeighborIndices(int index) const;
    glm::vec3 IndexToPosition(int index) const;
    std::pair<float, float> IndexToLatLong(int index) const;
};

} // namespace Core
} // namespace WorldGen
