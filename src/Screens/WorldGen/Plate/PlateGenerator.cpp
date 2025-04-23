#define _USE_MATH_DEFINES
#include <cmath>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "PlateGenerator.h"
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>
#include <algorithm>
#include <queue>

namespace WorldGen {

PlateGenerator::PlateGenerator(const PlanetParameters& parameters, uint64_t seed)
    : m_parameters(parameters)
    , m_random(seed)
{
}

std::vector<std::shared_ptr<TectonicPlate>> PlateGenerator::GeneratePlates()
{
    std::vector<std::shared_ptr<TectonicPlate>> plates;
    std::vector<glm::vec3> centers;
    
    // Generate plate centers using Poisson disk sampling
    GeneratePlateCenters(centers, m_parameters.numTectonicPlates);
    
    // Create plates with appropriate types
    for (int i = 0; i < centers.size(); ++i) {
        // Determine plate type - roughly 30% continental, 70% oceanic
        PlateType type = (m_random() % 100 < 30) ? PlateType::Continental : PlateType::Oceanic;
        
        // Create plate
        plates.push_back(std::make_shared<TectonicPlate>(i, type, centers[i]));
    }
    
    // Assign tiles to plates using Voronoi diagram
    AssignTilesToPlates(plates, m_parameters.resolution);
    
    // Generate initial plate movements
    GeneratePlateMovements(plates);
    
    // Detect plate boundaries
    DetectPlateBoundaries(plates);
    
    return plates;
}

void PlateGenerator::SimulatePlateMovement(
    std::vector<std::shared_ptr<TectonicPlate>>& plates,
    int simulationSteps)
{
    // Simulate plate movement over time
    for (int step = 0; step < simulationSteps; ++step) {
        for (auto& plate : plates) {
            // Apply small random variations to movement to simulate geological processes
            float variationScale = 0.01f;
            glm::vec3 variation(
                (m_random() % 2000 - 1000) / 1000.0f * variationScale,
                (m_random() % 2000 - 1000) / 1000.0f * variationScale,
                (m_random() % 2000 - 1000) / 1000.0f * variationScale
            );
            
            // Apply variation to movement vector
            glm::vec3 movement = plate->GetMovementVector() + variation;
            
            // Ensure movement stays on the sphere surface by projecting
            // the movement vector onto the tangent plane at the plate center
            glm::vec3 normal = glm::normalize(plate->GetCenter());
            movement = movement - normal * glm::dot(movement, normal);
            
            // Update plate movement
            plate->SetMovementVector(movement);
            
            // Apply small random variations to rotation rate
            float rotationVariation = (m_random() % 2000 - 1000) / 1000.0f * 0.005f;
            plate->SetRotationRate(plate->GetRotationRate() + rotationVariation);
        }
        
        // Update boundaries and stresses
        DetectPlateBoundaries(plates);
    }
}

void PlateGenerator::AnalyzeBoundaries(
    std::vector<std::shared_ptr<TectonicPlate>>& plates)
{
    // For each plate
    for (auto& plate : plates) {
        // For each boundary
        for (auto& boundary : plate->GetBoundaries()) {
            // Get the other plate
            auto& otherPlate = plates[boundary.plate2Index];
            
            // For each point along the boundary
            for (const auto& point : boundary.points) {
                // Determine boundary type
                boundary.type = DetermineBoundaryType(point, *plate, *otherPlate);
                
                // Calculate stress at boundary
                boundary.stress = CalculateStressAtBoundary(boundary, *plate, *otherPlate);
            }
        }
    }
}

std::vector<float> PlateGenerator::GenerateElevationData(
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    int resolution)
{
    // Create elevation grid
    int gridSize = resolution * resolution * 6; // 6 faces for cube mapping
    std::vector<float> elevationGrid(gridSize, 0.0f);
    
    // For each point in the grid
    for (int i = 0; i < gridSize; ++i) {
        // Convert grid index to 3D point on unit sphere
        // This is a simplified placeholder - actual implementation would use proper cube-to-sphere mapping
        float x = (i % resolution) / static_cast<float>(resolution) * 2.0f - 1.0f;
        float y = ((i / resolution) % resolution) / static_cast<float>(resolution) * 2.0f - 1.0f;
        float z = (i / (resolution * resolution)) / 6.0f * 2.0f - 1.0f;
        glm::vec3 point = glm::normalize(glm::vec3(x, y, z));
        
        // Calculate elevation at this point
        elevationGrid[i] = CalculateElevationAtPoint(point, plates);
    }
    
    return elevationGrid;
}

void PlateGenerator::GeneratePlateCenters(std::vector<glm::vec3>& centers, int numPlates) {
    centers.clear();
    
    // Minimum distance between plate centers (adjusted based on number of plates)
    float minDistance = 2.0f / std::sqrt(static_cast<float>(numPlates));
    
    // Generate first point randomly
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    glm::vec3 firstPoint;
    do {
        firstPoint = glm::vec3(
            dist(m_random),
            dist(m_random),
            dist(m_random)
        );
    } while (glm::length(firstPoint) < 0.001f); // Avoid zero vector
    
    // Normalize to put on unit sphere
    firstPoint = glm::normalize(firstPoint);
    centers.push_back(firstPoint);
    
    // Try to generate remaining points
    int attempts = 0;
    const int maxAttempts = numPlates * 100; // Limit attempts to avoid infinite loop
    
    while (centers.size() < numPlates && attempts < maxAttempts) {
        // Generate random point on sphere
        glm::vec3 candidate;
        do {
            candidate = glm::vec3(
                dist(m_random),
                dist(m_random),
                dist(m_random)
            );
        } while (glm::length(candidate) < 0.001f);
        
        candidate = glm::normalize(candidate);
        
        // Check distance to existing points
        bool tooClose = false;
        for (const auto& center : centers) {
            // Use angle between vectors as distance measure on sphere
            float angle = glm::angle(center, candidate);
            if (angle < minDistance) {
                tooClose = true;
                break;
            }
        }
        
        // If not too close to any existing point, add it
        if (!tooClose) {
            centers.push_back(candidate);
        }
        
        attempts++;
    }
}

void PlateGenerator::AssignTilesToPlates(std::vector<std::shared_ptr<TectonicPlate>>& plates, int resolution) {
    // For each point on the sphere
    for (int i = 0; i < resolution; ++i) {
        for (int j = 0; j < resolution; ++j) {
            // Convert grid coordinates to spherical coordinates
            float theta = 2.0f * M_PI * i / resolution;
            float phi = M_PI * j / resolution;
            
            glm::vec3 position(
                sin(phi) * cos(theta),
                sin(phi) * sin(theta),
                cos(phi)
            );
            
            // Find closest plate using spherical distance
            int closestPlate = 0;
            float minAngle = glm::angle(position, plates[0]->GetCenter());
            
            for (size_t k = 1; k < plates.size(); ++k) {
                float angle = glm::angle(position, plates[k]->GetCenter());
                if (angle < minAngle) {
                    minAngle = angle;
                    closestPlate = static_cast<int>(k);
                }
            }
            
            // Assign tile to closest plate
            plates[closestPlate]->AddTile(i * resolution + j);
        }
    }
}

void PlateGenerator::DetectPlateBoundaries(std::vector<std::shared_ptr<TectonicPlate>>& plates) {
    // Clear existing boundaries
    for (auto& plate : plates) {
        plate->GetBoundaries().clear();
    }
    
    // For each pair of plates
    for (size_t i = 0; i < plates.size(); ++i) {
        for (size_t j = i + 1; j < plates.size(); ++j) {
            PlateBoundary boundary;
            boundary.plate1Index = static_cast<int>(i);
            boundary.plate2Index = static_cast<int>(j);
            
            // Find points along great circle between plate centers
            glm::vec3 center1 = plates[i]->GetCenter();
            glm::vec3 center2 = plates[j]->GetCenter();
            
            // Calculate rotation axis and angle
            glm::vec3 axis = glm::normalize(glm::cross(center1, center2));
            float angle = glm::angle(center1, center2);
            
            // Generate points along the great circle
            const int numPoints = 20; // More points for smoother boundaries
            for (int k = 0; k < numPoints; ++k) {
                float t = static_cast<float>(k) / (numPoints - 1);
                float currentAngle = angle * t;
                
                // Rotate center1 around axis by currentAngle
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), currentAngle, axis);
                glm::vec3 point = glm::vec3(rotation * glm::vec4(center1, 1.0f));
                
                boundary.points.push_back(glm::normalize(point));
            }
            
            // Determine boundary type and calculate initial stress
            boundary.type = DetermineBoundaryType(boundary.points[0], *plates[i], *plates[j]);
            boundary.stress = CalculateStressAtBoundary(boundary, *plates[i], *plates[j]);
            
            // Add boundary to both plates
            plates[i]->AddBoundary(boundary);
            
            // Swap indices for second plate's copy
            std::swap(boundary.plate1Index, boundary.plate2Index);
            plates[j]->AddBoundary(boundary);
        }
    }
}

BoundaryType PlateGenerator::DetermineBoundaryType(
    const glm::vec3& point,
    const TectonicPlate& plate1,
    const TectonicPlate& plate2)
{
    // Calculate movement vectors at the boundary point
    glm::vec3 movement1 = plate1.GetMovementVector();
    glm::vec3 movement2 = plate2.GetMovementVector();
    
    // Calculate relative movement
    glm::vec3 relativeMovement = movement2 - movement1;
    
    // Calculate normal to the boundary (perpendicular to both point and tangent)
    glm::vec3 tangent = glm::normalize(glm::cross(point, plate2.GetCenter() - plate1.GetCenter()));
    glm::vec3 normal = glm::normalize(glm::cross(tangent, point));
    
    // Project relative movement onto normal and tangent
    float normalComponent = glm::dot(relativeMovement, normal);
    float tangentComponent = glm::dot(relativeMovement, tangent);
    
    // Determine boundary type based on movement components
    if (std::abs(normalComponent) > std::abs(tangentComponent)) {
        return normalComponent > 0 ? BoundaryType::Divergent : BoundaryType::Convergent;
    } else {
        return BoundaryType::Transform;
    }
}

float PlateGenerator::CalculateStressAtBoundary(
    const PlateBoundary& boundary,
    const TectonicPlate& plate1,
    const TectonicPlate& plate2)
{
    float totalStress = 0.0f;
    
    for (const auto& point : boundary.points) {
        // Calculate relative movement at this point
        glm::vec3 movement1 = plate1.GetMovementVector();
        glm::vec3 movement2 = plate2.GetMovementVector();
        glm::vec3 relativeMovement = movement2 - movement1;
        
        // Base stress is proportional to relative movement magnitude
        float pointStress = glm::length(relativeMovement);
        
        // Adjust stress based on boundary type and plate types
        switch (boundary.type) {
            case BoundaryType::Convergent:
                pointStress *= 1.5f;
                if (plate1.GetType() == PlateType::Continental && 
                    plate2.GetType() == PlateType::Continental) {
                    pointStress *= 2.0f; // Higher stress for colliding continents
                }
                break;
            case BoundaryType::Divergent:
                pointStress *= 0.8f;
                break;
            case BoundaryType::Transform:
                pointStress *= 1.0f;
                break;
        }
        
        totalStress += pointStress;
    }
    
    return boundary.points.empty() ? 0.0f : totalStress / boundary.points.size();
}

void PlateGenerator::GeneratePlateMovements(std::vector<std::shared_ptr<TectonicPlate>>& plates) {
    for (auto& plate : plates) {
        // Generate random movement direction tangent to sphere at plate center
        glm::vec3 normal = glm::normalize(plate->GetCenter());
        
        // Generate random vector
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        glm::vec3 randomVec(
            dist(m_random),
            dist(m_random),
            dist(m_random)
        );
        
        // Project onto tangent plane
        glm::vec3 movement = randomVec - normal * glm::dot(randomVec, normal);
        
        // Normalize and scale
        if (glm::length(movement) > 0.001f) {
            movement = glm::normalize(movement) * 0.005f; // Reduced movement speed
        }
        
        plate->SetMovementVector(movement);
        
        // Set random rotation rate (-0.002 to 0.002)
        float rotationRate = (m_random() % 2000 - 1000) / 500000.0f;
        plate->SetRotationRate(rotationRate);
    }
}

float PlateGenerator::CalculateElevationAtPoint(
    const glm::vec3& point,
    const std::vector<std::shared_ptr<TectonicPlate>>& plates)
{
    // Find which plate this point belongs to
    int plateIndex = -1;
    float minDistance = std::numeric_limits<float>::max();
    
    for (int i = 0; i < plates.size(); ++i) {
        float distance = glm::angle(point, plates[i]->GetCenter());
        if (distance < minDistance) {
            minDistance = distance;
            plateIndex = i;
        }
    }
    
    if (plateIndex == -1) {
        return 0.0f; // Fallback
    }
    
    // Get base elevation from plate type
    float elevation = plates[plateIndex]->GetBaseElevation();
    
    // Check if point is near a boundary
    for (const auto& boundary : plates[plateIndex]->GetBoundaries()) {
        for (const auto& boundaryPoint : boundary.points) {
            float distance = glm::angle(point, boundaryPoint);
            
            // If point is near boundary
            if (distance < 0.1f) { // Arbitrary threshold
                float influence = 1.0f - (distance / 0.1f); // 1.0 at boundary, 0.0 at threshold
                
                // Adjust elevation based on boundary type and stress
                switch (boundary.type) {
                    case BoundaryType::Convergent: {
                        // Create mountains at convergent boundaries
                        float mountainHeight = boundary.stress * 0.5f * influence;
                        
                        // Higher mountains when continental plates collide
                        auto& otherPlate = plates[boundary.plate2Index];
                        if (plates[plateIndex]->GetType() == PlateType::Continental && 
                            otherPlate->GetType() == PlateType::Continental) {
                            mountainHeight *= 2.0f;
                        }
                        // Oceanic-continental creates smaller mountains but deeper trenches
                        else if (plates[plateIndex]->GetType() != otherPlate->GetType()) {
                            if (plates[plateIndex]->GetType() == PlateType::Oceanic) {
                                // Oceanic plate gets pushed under, creating trench
                                mountainHeight = -mountainHeight * 1.5f;
                            }
                        }
                        
                        elevation += mountainHeight;
                        break;
                    }
                    
                    case BoundaryType::Divergent: {
                        // Create rifts at divergent boundaries
                        float riftDepth = -boundary.stress * 0.3f * influence;
                        
                        // Deeper rifts in oceanic plates (mid-ocean ridges)
                        if (plates[plateIndex]->GetType() == PlateType::Oceanic) {
                            riftDepth *= 1.5f;
                        }
                        
                        elevation += riftDepth;
                        break;
                    }
                    
                    case BoundaryType::Transform: {
                        // Slight elevation changes at transform boundaries
                        float variation = boundary.stress * 0.1f * influence;
                        
                        // Random sign based on position
                        float hash = std::sin(point.x * 12.9898f + point.y * 78.233f + point.z * 45.164f) * 43758.5453f;
                        hash = hash - std::floor(hash);
                        if (hash > 0.5f) {
                            variation = -variation;
                        }
                        
                        elevation += variation;
                        break;
                    }
                }
            }
        }
    }
    
    return elevation;
}

} // namespace WorldGen 