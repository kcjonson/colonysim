#include "WorldGen/Core/Terrain/RiverGenerator.h"
#include <cmath>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <random>

namespace WorldGen {
namespace Core {

// Constants
constexpr float PI = 3.14159265358979323846f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// River implementation
River::River(int id)
    : m_id(id)
{
}

int River::AddSegment(const RiverSegment& segment)
{
    m_segments.push_back(segment);
    return static_cast<int>(m_segments.size() - 1);
}

float River::GetTotalLength() const
{
    float totalLength = 0.0f;
    for (const auto& segment : m_segments) {
        totalLength += glm::distance(segment.startPoint, segment.endPoint);
    }
    return totalLength;
}

glm::vec3 River::GetSource() const
{
    if (m_segments.empty()) {
        return glm::vec3(0.0f);
    }
    
    // Find segment with no incoming connections
    for (const auto& segment : m_segments) {
        bool isSource = true;
        for (const auto& otherSegment : m_segments) {
            if (otherSegment.nextSegmentIndex >= 0 && 
                &m_segments[otherSegment.nextSegmentIndex] == &segment) {
                isSource = false;
                break;
            }
        }
        
        if (isSource) {
            return segment.startPoint;
        }
    }
    
    // Fallback to first segment
    return m_segments[0].startPoint;
}

glm::vec3 River::GetMouth() const
{
    if (m_segments.empty()) {
        return glm::vec3(0.0f);
    }
    
    // Find segment with no outgoing connections
    for (const auto& segment : m_segments) {
        if (segment.nextSegmentIndex < 0) {
            return segment.endPoint;
        }
    }
    
    // Fallback to last segment
    return m_segments.back().endPoint;
}

float River::GetAverageFlow() const
{
    if (m_segments.empty()) {
        return 0.0f;
    }
    
    float totalFlow = 0.0f;
    for (const auto& segment : m_segments) {
        totalFlow += segment.flow;
    }
    
    return totalFlow / m_segments.size();
}

// Lake implementation
Lake::Lake(int id, const glm::vec3& center)
    : m_id(id)
    , m_center(center)
    , m_depth(0.0f)
    , m_outflowRiver(-1)
{
}

void Lake::AddBoundaryPoint(const glm::vec3& point)
{
    m_boundaryPoints.push_back(point);
}

float Lake::GetArea() const
{
    if (m_boundaryPoints.size() < 3) {
        return 0.0f;
    }
    
    // Calculate approximate area using spherical polygon formula
    // This is a simplified calculation - a real implementation would be more accurate
    float area = 0.0f;
    
    for (size_t i = 0; i < m_boundaryPoints.size(); ++i) {
        const glm::vec3& p1 = m_boundaryPoints[i];
        const glm::vec3& p2 = m_boundaryPoints[(i + 1) % m_boundaryPoints.size()];
        
        // Calculate angle between vectors
        float angle = std::acos(std::max(-1.0f, std::min(1.0f, glm::dot(p1, p2))));
        
        // Add to area
        area += angle;
    }
    
    // Adjust for spherical excess
    area -= (m_boundaryPoints.size() - 2) * PI;
    
    return std::abs(area);
}

void Lake::AddInflowRiver(int riverId)
{
    // Check if river is already in the list
    if (std::find(m_inflowRivers.begin(), m_inflowRivers.end(), riverId) == m_inflowRivers.end()) {
        m_inflowRivers.push_back(riverId);
    }
}

// RiverGenerator implementation
RiverGenerator::RiverGenerator(const PlanetParameters& parameters)
    : m_parameters(parameters)
    , m_resolution(0)
{
}

RiverGenerator::~RiverGenerator()
{
}

std::pair<std::vector<std::shared_ptr<River>>, std::vector<std::shared_ptr<Lake>>> 
RiverGenerator::GenerateRiversAndLakes(
    const std::vector<float>& elevationData,
    const std::vector<float>& precipitationData,
    int resolution,
    std::shared_ptr<ProgressTracker> progressTracker)
{
    // Initialize progress tracking if provided
    if (progressTracker) {
        progressTracker->StartPhase("Generating Rivers and Lakes", 0.2f);
    }
    
    m_resolution = resolution;
    
    // Vectors to hold results
    std::vector<std::shared_ptr<River>> rivers;
    std::vector<std::shared_ptr<Lake>> lakes;
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.1f, "Calculating water flow");
    }
    
    // Calculate water flow based on precipitation and elevation
    std::vector<float> flowData = CalculateWaterFlow(elevationData, precipitationData);
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.3f, "Determining flow directions");
    }
    
    // Calculate flow directions
    std::vector<int> flowDirections = CalculateFlowDirections(elevationData);
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.5f, "Tracing river paths");
    }
    
    // Trace river paths
    TraceRiverPaths(flowData, flowDirections, rivers);
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.7f, "Identifying lakes");
    }
    
    // Identify lakes
    IdentifyLakes(elevationData, flowData, lakes);
    
    // Update progress
    if (progressTracker) {
        progressTracker->UpdateProgress(0.9f, "Connecting rivers and lakes");
    }
    
    // Connect rivers and lakes
    ConnectRiversAndLakes(rivers, lakes);
    
    // Complete progress phase
    if (progressTracker) {
        progressTracker->CompletePhase();
    }
    
    return std::make_pair(rivers, lakes);
}

void RiverGenerator::ApplyErosion(
    std::vector<float>& elevationData,
    const std::vector<std::shared_ptr<River>>& rivers,
    int resolution,
    std::shared_ptr<ProgressTracker> progressTracker)
{
    // Initialize progress tracking if provided
    if (progressTracker) {
        progressTracker->StartPhase("Applying River Erosion", 0.1f);
    }
    
    m_resolution = resolution;
    
    // For each river
    for (size_t i = 0; i < rivers.size(); ++i) {
        const auto& river = rivers[i];
        
        // For each segment
        for (const auto& segment : river->GetSegments()) {
            // Calculate erosion strength based on flow
            float erosionStrength = segment.flow * 0.01f; // Scale factor
            
            // Calculate points along the segment
            glm::vec3 direction = segment.endPoint - segment.startPoint;
            float length = glm::length(direction);
            
            if (length < 0.001f) continue;
            
            direction = glm::normalize(direction);
            
            // Number of points depends on segment length
            int numPoints = std::max(2, static_cast<int>(length * 100.0f));
            
            // Apply erosion along the segment
            for (int j = 0; j < numPoints; ++j) {
                float t = j / static_cast<float>(numPoints - 1);
                glm::vec3 point = segment.startPoint + direction * t * length;
                
                // Convert point to grid index
                // This is a simplified conversion - a real implementation would use proper mapping
                auto [latitude, longitude] = IndexToLatLong(0); // Placeholder
                int index = GetGridIndex(latitude, longitude);
                
                if (index >= 0 && index < elevationData.size()) {
                    // Apply erosion - deeper for higher flow
                    float erosionAmount = erosionStrength * (1.0f - t * 0.5f); // Less erosion downstream
                    elevationData[index] -= erosionAmount;
                    
                    // Apply erosion to neighboring points with decreasing strength
                    auto neighbors = GetNeighborIndices(index);
                    for (int neighborIndex : neighbors) {
                        if (neighborIndex >= 0 && neighborIndex < elevationData.size()) {
                            elevationData[neighborIndex] -= erosionAmount * 0.5f;
                        }
                    }
                }
            }
        }
        
        // Update progress
        if (progressTracker) {
            float progress = static_cast<float>(i + 1) / rivers.size();
            progressTracker->UpdateProgress(progress, 
                "Eroding river " + std::to_string(i + 1) + " of " + std::to_string(rivers.size()));
        }
    }
    
    // Complete progress phase
    if (progressTracker) {
        progressTracker->CompletePhase();
    }
}

std::vector<float> RiverGenerator::CalculateWaterFlow(
    const std::vector<float>& elevationData,
    const std::vector<float>& precipitationData)
{
    // Initialize flow data with precipitation
    std::vector<float> flowData = precipitationData;
    
    // Calculate flow accumulation
    // This is a simplified implementation - a real implementation would use a more sophisticated algorithm
    
    // Create a priority queue to process cells from highest to lowest
    struct Cell {
        int index;
        float elevation;
        
        bool operator<(const Cell& other) const {
            return elevation < other.elevation; // Note: priority_queue is a max heap
        }
    };
    
    std::priority_queue<Cell> queue;
    
    // Add all cells to queue
    for (size_t i = 0; i < elevationData.size(); ++i) {
        queue.push({static_cast<int>(i), elevationData[i]});
    }
    
    // Process cells from highest to lowest
    while (!queue.empty()) {
        Cell cell = queue.top();
        queue.pop();
        
        // Get neighbors
        auto neighbors = GetNeighborIndices(cell.index);
        
        // Find lowest neighbor
        int lowestNeighbor = -1;
        float lowestElevation = cell.elevation;
        
        for (int neighborIndex : neighbors) {
            if (neighborIndex >= 0 && neighborIndex < elevationData.size() && 
                elevationData[neighborIndex] < lowestElevation) {
                lowestNeighbor = neighborIndex;
                lowestElevation = elevationData[neighborIndex];
            }
        }
        
        // If there's a lower neighbor, flow to it
        if (lowestNeighbor >= 0) {
            // Transfer flow
            flowData[lowestNeighbor] += flowData[cell.index];
        }
    }
    
    return flowData;
}

std::vector<int> RiverGenerator::CalculateFlowDirections(
    const std::vector<float>& elevationData)
{
    // Initialize flow directions
    // -1 means no outflow (sink)
    std::vector<int> flowDirections(elevationData.size(), -1);
    
    // For each cell
    for (size_t i = 0; i < elevationData.size(); ++i) {
        // Get neighbors
        auto neighbors = GetNeighborIndices(static_cast<int>(i));
        
        // Find lowest neighbor
        int lowestNeighbor = -1;
        float lowestElevation = elevationData[i];
        
        for (int neighborIndex : neighbors) {
            if (neighborIndex >= 0 && neighborIndex < elevationData.size() && 
                elevationData[neighborIndex] < lowestElevation) {
                lowestNeighbor = neighborIndex;
                lowestElevation = elevationData[neighborIndex];
            }
        }
        
        // Set flow direction
        flowDirections[i] = lowestNeighbor;
    }
    
    return flowDirections;
}

void RiverGenerator::TraceRiverPaths(
    const std::vector<float>& flowData,
    const std::vector<int>& flowDirections,
    std::vector<std::shared_ptr<River>>& rivers)
{
    // Threshold for river formation
    float riverThreshold = 0.01f; // Arbitrary threshold
    
    // Set of cells already processed
    std::unordered_set<int> processedCells;
    
    // For each cell
    for (size_t i = 0; i < flowData.size(); ++i) {
        // Skip if already processed
        if (processedCells.find(static_cast<int>(i)) != processedCells.end()) {
            continue;
        }
        
        // Skip if flow is below threshold
        if (flowData[i] < riverThreshold) {
            continue;
        }
        
        // Create a new river
        auto river = std::make_shared<River>(static_cast<int>(rivers.size()));
        
        // Trace path from this cell
        int currentCell = static_cast<int>(i);
        int prevSegmentIndex = -1;
        
        while (currentCell >= 0 && 
               processedCells.find(currentCell) == processedCells.end()) {
            // Mark as processed
            processedCells.insert(currentCell);
            
            // Get position
            glm::vec3 currentPos = IndexToPosition(currentCell);
            
            // Get next cell
            int nextCell = flowDirections[currentCell];
            
            // Create segment
            RiverSegment segment;
            segment.startPoint = currentPos;
            segment.flow = flowData[currentCell];
            segment.width = CalculateRiverWidth(segment.flow);
            
            if (nextCell >= 0) {
                // Set end point to next cell
                segment.endPoint = IndexToPosition(nextCell);
                
                // Add segment to river
                int segmentIndex = river->AddSegment(segment);
                
                // Connect to previous segment
                if (prevSegmentIndex >= 0) {
                    river->GetSegments()[prevSegmentIndex].nextSegmentIndex = segmentIndex;
                }
                
                // Update for next iteration
                prevSegmentIndex = segmentIndex;
                currentCell = nextCell;
            } else {
                // End of river (sink)
                segment.endPoint = currentPos; // Same as start for now
                segment.nextSegmentIndex = -1;
                
                // Add final segment
                int segmentIndex = river->AddSegment(segment);
                
                // Connect to previous segment
                if (prevSegmentIndex >= 0) {
                    river->GetSegments()[prevSegmentIndex].nextSegmentIndex = segmentIndex;
                }
                
                break;
            }
        }
        
        // Add river if it has segments
        if (!river->GetSegments().empty()) {
            rivers.push_back(river);
        }
    }
}

void RiverGenerator::IdentifyLakes(
    const std::vector<float>& elevationData,
    const std::vector<float>& flowData,
    std::vector<std::shared_ptr<Lake>>& lakes)
{
    // Identify depressions (local minima)
    std::vector<int> depressions;
    
    for (size_t i = 0; i < elevationData.size(); ++i) {
        // Get neighbors
        auto neighbors = GetNeighborIndices(static_cast<int>(i));
        
        // Check if this is a local minimum
        bool isMinimum = true;
        for (int neighborIndex : neighbors) {
            if (neighborIndex >= 0 && neighborIndex < elevationData.size() && 
                elevationData[neighborIndex] < elevationData[i]) {
                isMinimum = false;
                break;
            }
        }
        
        // If it's a minimum and has significant flow, it's a potential lake
        if (isMinimum && flowData[i] > 0.05f) { // Arbitrary threshold
            depressions.push_back(static_cast<int>(i));
        }
    }
    
    // Create lakes at depressions
    for (int depressionIndex : depressions) {
        // Create lake
        auto lake = std::make_shared<Lake>(
            static_cast<int>(lakes.size()),
            IndexToPosition(depressionIndex)
        );
        
        // Set depth based on flow
        lake->SetDepth(flowData[depressionIndex] * 0.1f); // Arbitrary scale
        
        // Find boundary points
        // This is a simplified approach - a real implementation would use a more sophisticated algorithm
        auto neighbors = GetNeighborIndices(depressionIndex);
        for (int neighborIndex : neighbors) {
            if (neighborIndex >= 0 && neighborIndex < elevationData.size()) {
                lake->AddBoundaryPoint(IndexToPosition(neighborIndex));
            }
        }
        
        // Add lake if it has boundary points
        if (!lake->GetBoundaryPoints().empty()) {
            lakes.push_back(lake);
        }
    }
}

void RiverGenerator::ConnectRiversAndLakes(
    std::vector<std::shared_ptr<River>>& rivers,
    std::vector<std::shared_ptr<Lake>>& lakes)
{
    // For each lake
    for (auto& lake : lakes) {
        // Find rivers that flow into this lake
        for (auto& river : rivers) {
            // Check if river ends near lake
            glm::vec3 riverMouth = river->GetMouth();
            float distance = glm::distance(riverMouth, lake->GetCenter());
            
            // If river ends near lake, it flows into the lake
            if (distance < 0.1f) { // Arbitrary threshold
                lake->AddInflowRiver(river->GetId());
            }
        }
        
        // Create outflow river if lake has inflows
        if (!lake->GetInflowRivers().empty()) {
            // Create a new river
            auto outflowRiver = std::make_shared<River>(static_cast<int>(rivers.size()));
            
            // Create first segment starting at lake
            RiverSegment segment;
            segment.startPoint = lake->GetCenter();
            
            // Calculate flow as sum of inflows
            float totalFlow = 0.0f;
            for (int inflowId : lake->GetInflowRivers()) {
                if (inflowId >= 0 && inflowId < rivers.size()) {
                    totalFlow += rivers[inflowId]->GetAverageFlow();
                }
            }
            
            segment.flow = totalFlow;
            segment.width = CalculateRiverWidth(segment.flow);
            
            // Find lowest point on lake boundary for outflow
            float lowestElevation = std::numeric_limits<float>::max();
            glm::vec3 outflowPoint;
            
            for (const auto& point : lake->GetBoundaryPoints()) {
                // Convert to grid index
                auto [latitude, longitude] = IndexToLatLong(0); // Placeholder
                int index = GetGridIndex(latitude, longitude);
                
                if (index >= 0 && index < rivers.size()) {
                    // Check elevation
                    // In a real implementation, this would use the actual elevation data
                    float elevation = 0.0f; // Placeholder
                    
                    if (elevation < lowestElevation) {
                        lowestElevation = elevation;
                        outflowPoint = point;
                    }
                }
            }
            
            // Set end point
            segment.endPoint = outflowPoint;
            segment.nextSegmentIndex = -1;
            
            // Add segment to river
            outflowRiver->AddSegment(segment);
            
            // Set as lake's outflow
            lake->SetOutflowRiver(outflowRiver->GetId());
            
            // Add river
            rivers.push_back(outflowRiver);
        }
    }
}

float RiverGenerator::CalculateRiverWidth(float flow) const
{
    // Calculate river width based on flow
    // This is a simplified model - a real implementation would be more sophisticated
    return std::max(0.001f, std::min(0.1f, flow * 0.05f)); // Arbitrary scale
}

int RiverGenerator::GetGridIndex(float latitude, float longitude) const
{
    if (m_resolution == 0) return -1;
    
    // Convert latitude/longitude to grid coordinates
    // This is a simplified conversion - a real implementation would use proper mapping
    
    // Normalize latitude and longitude to 0-1 range
    float normLat = (latitude + 90.0f) / 180.0f;
    float normLon = (longitude + 180.0f) / 360.0f;
    
    // Convert to grid coordinates
    int x = static_cast<int>(normLon * m_resolution);
    int y = static_cast<int>(normLat * m_resolution);
    
    // Determine cube face (simplified)
    int face = 0;
    if (longitude >= -180.0f && longitude < -90.0f) face = 0;
    else if (longitude >= -90.0f && longitude < 0.0f) face = 1;
    else if (longitude >= 0.0f && longitude < 90.0f) face = 2;
    else face = 3;
    
    // Special case for poles
    if (latitude > 80.0f) face = 4;
    else if (latitude < -80.0f) face = 5;
    
    // Calculate index
    int index = x + y * m_resolution + face * m_resolution * m_resolution;
    
    // Bounds check
    if (index >= 0 && index < m_resolution * m_resolution * 6) {
        return index;
    }
    
    return -1;
}

std::vector<int> RiverGenerator::GetNeighborIndices(int index) const
{
    if (m_resolution == 0) return {};
    
    // Convert index to grid coordinates
    int face = index / (m_resolution * m_resolution);
    int remainder = index % (m_resolution * m_resolution);
    int y = remainder / m_resolution;
    int x = remainder % m_resolution;
    
    // Get neighbors (4-connected)
    std::vector<int> neighbors;
    
    // East neighbor
    if (x < m_resolution - 1) {
        neighbors.push_back(index + 1);
    } else {
        // Wrap to next face (simplified)
        int nextFace = (face + 1) % 4;
        int nextX = 0;
        int nextY = y;
        neighbors.push_back(nextX + nextY * m_resolution + nextFace * m_resolution * m_resolution);
    }
    
    // West neighbor
    if (x > 0) {
        neighbors.push_back(index - 1);
    } else {
        // Wrap to previous face (simplified)
        int prevFace = (face + 3) % 4;
        int prevX = m_resolution - 1;
        int prevY = y;
        neighbors.push_back(prevX + prevY * m_resolution + prevFace * m_resolution * m_resolution);
    }
    
    // North neighbor
    if (y < m_resolution - 1) {
        neighbors.push_back(index + m_resolution);
    } else {
        // Wrap to top face (simplified)
        int topFace = 4;
        int topX = x;
        int topY = 0;
        neighbors.push_back(topX + topY * m_resolution + topFace * m_resolution * m_resolution);
    }
    
    // South neighbor
    if (y > 0) {
        neighbors.push_back(index - m_resolution);
    } else {
        // Wrap to bottom face (simplified)
        int bottomFace = 5;
        int bottomX = x;
        int bottomY = m_resolution - 1;
        neighbors.push_back(bottomX + bottomY * m_resolution + bottomFace * m_resolution * m_resolution);
    }
    
    return neighbors;
}

glm::vec3 RiverGenerator::IndexToPosition(int index) const
{
    // Convert grid index to 3D point on unit sphere
    // This is a simplified conversion - a real implementation would use proper mapping
    
    // Convert index to grid coordinates
    int face = index / (m_resolution * m_resolution);
    int remainder = index % (m_resolution * m_resolution);
    int y = remainder / m_resolution;
    int x = remainder % m_resolution;
    
    // Convert to normalized coordinates (-1 to 1)
    float nx = (x / static_cast<float>(m_resolution)) * 2.0f - 1.0f;
    float ny = (y / static_cast<float>(m_resolution)) * 2.0f - 1.0f;
    
    // Convert to 3D point based on face
    glm::vec3 point;
    
    switch (face) {
        case 0: // Front face
            point = glm::vec3(nx, ny, 1.0f);
            break;
        case 1: // Right face
            point = glm::vec3(1.0f, ny, -nx);
            break;
        case 2: // Back face
            point = glm::vec3(-nx, ny, -1.0f);
            break;
        case 3: // Left face
            point = glm::vec3(-1.0f, ny, nx);
            break;
        case 4: // Top face
            point = glm::vec3(nx, 1.0f, -ny);
            break;
        case 5: // Bottom face
            point = glm::vec3(nx, -1.0f, ny);
            break;
        default:
            point = glm::vec3(0.0f);
    }
    
    // Normalize to put on unit sphere
    return glm::normalize(point);
}

std::pair<float, float> RiverGenerator::IndexToLatLong(int index) const
{
    // Convert grid index to latitude/longitude
    // This is a simplified conversion - a real implementation would use proper mapping
    
    // Convert to 3D point
    glm::vec3 point = IndexToPosition(index);
    
    // Convert to latitude/longitude
    float latitude = std::asin(point.y) * RAD_TO_DEG;
    float longitude = std::atan2(point.x, point.z) * RAD_TO_DEG;
    
    return std::make_pair(latitude, longitude);
}

} // namespace Core
} // namespace WorldGen
