#include "WorldGen/Core/TectonicPlates/PlateGenerator.h"
#include <glm/gtx/vector_angle.hpp>
#include <algorithm>
#include <queue>

namespace WorldGen {
namespace Core {

PlateGenerator::PlateGenerator(const PlanetParameters& parameters, uint64_t seed)
    : m_parameters(parameters)
    , m_random(seed)
{
}

PlateGenerator::~PlateGenerator()
{
}

std::vector<std::shared_ptr<TectonicPlate>> PlateGenerator::GeneratePlates(
    std::shared_ptr<ProgressTracker> progressTracker)
{
    // Initialize progress tracking if provided
    if (progressTracker) {
        progressTracker->StartPhase("Generating Tectonic Plates", 0.1f);
    }
    
    // Create vector to hold plates
    std::vector<std::shared_ptr<TectonicPlate>> plates;
    
    // Generate plate centers using Poisson disk sampling
    std::vector<glm::vec3> centers;
    GeneratePlateCenters(centers, m_parameters.numTectonicPlates);
    
    // Create plates with appropriate types
    for (int i = 0; i < centers.size(); ++i) {
        // Determine plate type - roughly 30% continental, 70% oceanic
        PlateType type = (m_random() % 100 < 30) ? PlateType::Continental : PlateType::Oceanic;
        
        // Create plate
        auto plate = std::make_shared<TectonicPlate>(i, type, centers[i]);
        plates.push_back(plate);
        
        // Update progress
        if (progressTracker) {
            float progress = static_cast<float>(i + 1) / centers.size();
            progressTracker->UpdateProgress(progress, 
                "Creating plate " + std::to_string(i + 1) + " of " + std::to_string(centers.size()));
        }
    }
    
    // Assign tiles to plates using Voronoi diagram
    AssignTilesToPlates(plates, m_parameters.resolution);
    
    // Generate plate movements
    GeneratePlateMovements(plates);
    
    // Detect plate boundaries
    DetectPlateBoundaries(plates);
    
    // Complete progress phase
    if (progressTracker) {
        progressTracker->CompletePhase();
    }
    
    return plates;
}

void PlateGenerator::SimulatePlateMovement(
    std::vector<std::shared_ptr<TectonicPlate>>& plates,
    int simulationSteps,
    std::shared_ptr<ProgressTracker> progressTracker)
{
    // Initialize progress tracking if provided
    if (progressTracker) {
        progressTracker->StartPhase("Simulating Plate Movement", 0.15f);
    }
    
    // Simulate plate movement over time
    for (int step = 0; step < simulationSteps; ++step) {
        // For each plate
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
        
        // Update progress
        if (progressTracker) {
            float progress = static_cast<float>(step + 1) / simulationSteps;
            progressTracker->UpdateProgress(progress, 
                "Simulation step " + std::to_string(step + 1) + " of " + std::to_string(simulationSteps));
        }
    }
    
    // Complete progress phase
    if (progressTracker) {
        progressTracker->CompletePhase();
    }
}

void PlateGenerator::AnalyzeBoundaries(
    std::vector<std::shared_ptr<TectonicPlate>>& plates,
    std::shared_ptr<ProgressTracker> progressTracker)
{
    // Initialize progress tracking if provided
    if (progressTracker) {
        progressTracker->StartPhase("Analyzing Plate Boundaries", 0.1f);
    }
    
    // Count total boundaries for progress tracking
    int totalBoundaries = 0;
    for (const auto& plate : plates) {
        totalBoundaries += plate->GetBoundaries().size();
    }
    
    int processedBoundaries = 0;
    
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
            
            // Update progress
            if (progressTracker) {
                processedBoundaries++;
                float progress = static_cast<float>(processedBoundaries) / totalBoundaries;
                progressTracker->UpdateProgress(progress, 
                    "Analyzing boundary " + std::to_string(processedBoundaries) + 
                    " of " + std::to_string(totalBoundaries));
            }
        }
    }
    
    // Complete progress phase
    if (progressTracker) {
        progressTracker->CompletePhase();
    }
}

std::vector<float> PlateGenerator::GenerateElevationData(
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    int resolution,
    std::shared_ptr<ProgressTracker> progressTracker)
{
    // Initialize progress tracking if provided
    if (progressTracker) {
        progressTracker->StartPhase("Generating Elevation Data", 0.2f);
    }
    
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
        
        // Update progress
        if (progressTracker && i % 1000 == 0) {
            float progress = static_cast<float>(i) / gridSize;
            progressTracker->UpdateProgress(progress, 
                "Calculating elevation for point " + std::to_string(i) + 
                " of " + std::to_string(gridSize));
        }
    }
    
    // Complete progress phase
    if (progressTracker) {
        progressTracker->CompletePhase();
    }
    
    return elevationGrid;
}

void PlateGenerator::GeneratePlateCenters(std::vector<glm::vec3>& centers, int numPlates)
{
    // This is a simplified implementation of Poisson disk sampling on a sphere
    // A full implementation would use a more sophisticated algorithm
    
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
    
    // If we couldn't generate enough points, adjust the existing ones
    if (centers.size() < numPlates) {
        // This is a fallback to ensure we have the requested number of plates
        // In a real implementation, you might want to use a different approach
        while (centers.size() < numPlates) {
            // Take an existing center and perturb it slightly
            int index = m_random() % centers.size();
            glm::vec3 base = centers[index];
            
            // Add small random offset
            glm::vec3 offset(
                dist(m_random) * 0.1f,
                dist(m_random) * 0.1f,
                dist(m_random) * 0.1f
            );
            
            // Ensure the new point is on the sphere
            glm::vec3 newPoint = glm::normalize(base + offset);
            centers.push_back(newPoint);
        }
    }
}

void PlateGenerator::AssignTilesToPlates(std::vector<std::shared_ptr<TectonicPlate>>& plates, int resolution)
{
    // This is a simplified implementation of tile assignment
    // A full implementation would use a proper spherical Voronoi diagram
    
    // Total number of tiles
    int gridSize = resolution * resolution * 6; // 6 faces for cube mapping
    
    // For each tile
    for (int i = 0; i < gridSize; ++i) {
        // Convert grid index to 3D point on unit sphere (simplified)
        float x = (i % resolution) / static_cast<float>(resolution) * 2.0f - 1.0f;
        float y = ((i / resolution) % resolution) / static_cast<float>(resolution) * 2.0f - 1.0f;
        float z = (i / (resolution * resolution)) / 6.0f * 2.0f - 1.0f;
        glm::vec3 tilePos = glm::normalize(glm::vec3(x, y, z));
        
        // Find closest plate center
        int closestPlate = 0;
        float minAngle = glm::angle(tilePos, plates[0]->GetCenter());
        
        for (int j = 1; j < plates.size(); ++j) {
            float angle = glm::angle(tilePos, plates[j]->GetCenter());
            if (angle < minAngle) {
                minAngle = angle;
                closestPlate = j;
            }
        }
        
        // Assign tile to closest plate
        plates[closestPlate]->AddTile(i);
    }
}

void PlateGenerator::GeneratePlateMovements(std::vector<std::shared_ptr<TectonicPlate>>& plates)
{
    // Generate random movement vectors for each plate
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
        
        // Normalize and scale by random speed
        if (glm::length(movement) > 0.001f) {
            movement = glm::normalize(movement);
            
            // Random speed between 0.001 and 0.01
            float speed = 0.001f + (m_random() % 1000) / 100000.0f;
            movement *= speed;
        }
        
        plate->SetMovementVector(movement);
        
        // Set random rotation rate (-0.005 to 0.005)
        float rotationRate = (m_random() % 2000 - 1000) / 200000.0f;
        plate->SetRotationRate(rotationRate);
    }
}

void PlateGenerator::DetectPlateBoundaries(std::vector<std::shared_ptr<TectonicPlate>>& plates)
{
    // This is a simplified implementation of boundary detection
    // A full implementation would use more sophisticated algorithms
    
    // Clear existing boundaries
    for (auto& plate : plates) {
        plate->GetBoundaries().clear();
    }
    
    // For each plate
    for (int i = 0; i < plates.size(); ++i) {
        auto& plate = plates[i];
        
        // For each tile in the plate
        for (int tileIndex : plate->GetTileIndices()) {
            // Convert tile index to position (simplified)
            int resolution = static_cast<int>(std::sqrt(m_parameters.resolution));
            float x = (tileIndex % resolution) / static_cast<float>(resolution) * 2.0f - 1.0f;
            float y = ((tileIndex / resolution) % resolution) / static_cast<float>(resolution) * 2.0f - 1.0f;
            float z = (tileIndex / (resolution * resolution)) / 6.0f * 2.0f - 1.0f;
            glm::vec3 tilePos = glm::normalize(glm::vec3(x, y, z));
            
            // Check neighboring tiles
            // This is a simplified approach - a real implementation would use actual tile neighbors
            for (int j = 0; j < plates.size(); ++j) {
                if (i == j) continue; // Skip same plate
                
                auto& otherPlate = plates[j];
                
                // Check if this tile is near the other plate
                // This is a very simplified check - a real implementation would be more sophisticated
                float minDistance = 0.1f; // Arbitrary threshold
                bool isNear = false;
                
                for (int otherTileIndex : otherPlate->GetTileIndices()) {
                    // Convert other tile index to position (simplified)
                    float ox = (otherTileIndex % resolution) / static_cast<float>(resolution) * 2.0f - 1.0f;
                    float oy = ((otherTileIndex / resolution) % resolution) / static_cast<float>(resolution) * 2.0f - 1.0f;
                    float oz = (otherTileIndex / (resolution * resolution)) / 6.0f * 2.0f - 1.0f;
                    glm::vec3 otherTilePos = glm::normalize(glm::vec3(ox, oy, oz));
                    
                    // Check distance
                    float angle = glm::angle(tilePos, otherTilePos);
                    if (angle < minDistance) {
                        isNear = true;
                        
                        // Create boundary point at midpoint
                        glm::vec3 midpoint = glm::normalize(tilePos + otherTilePos);
                        
                        // Check if we already have a boundary with this plate
                        bool boundaryExists = false;
                        for (auto& boundary : plate->GetBoundaries()) {
                            if (boundary.plate2Index == j) {
                                // Add point to existing boundary
                                boundary.points.push_back(midpoint);
                                boundaryExists = true;
                                break;
                            }
                        }
                        
                        // If no boundary exists, create a new one
                        if (!boundaryExists) {
                            PlateBoundary newBoundary;
                            newBoundary.plate1Index = i;
                            newBoundary.plate2Index = j;
                            newBoundary.type = BoundaryType::Transform; // Default, will be determined later
                            newBoundary.points.push_back(midpoint);
                            newBoundary.stress = 0.0f;
                            plate->AddBoundary(newBoundary);
                        }
                        
                        break;
                    }
                }
            }
        }
    }
}

BoundaryType PlateGenerator::DetermineBoundaryType(const glm::vec3& point, 
                                                 const TectonicPlate& plate1, 
                                                 const TectonicPlate& plate2)
{
    // Calculate movement vectors at the boundary point
    glm::vec3 movement1 = plate1.CalculateMovementAt(point);
    glm::vec3 movement2 = plate2.CalculateMovementAt(point);
    
    // Calculate relative movement
    glm::vec3 relativeMovement = movement2 - movement1;
    
    // Calculate normal to the boundary (perpendicular to both point and tangent)
    glm::vec3 tangent = glm::normalize(glm::cross(point, plate2.GetCenter() - plate1.GetCenter()));
    glm::vec3 normal = glm::normalize(glm::cross(tangent, point));
    
    // Project relative movement onto normal
    float normalComponent = glm::dot(relativeMovement, normal);
    
    // Project relative movement onto tangent
    float tangentComponent = glm::dot(relativeMovement, tangent);
    
    // Determine boundary type based on movement components
    if (std::abs(normalComponent) > std::abs(tangentComponent)) {
        // Movement is primarily normal to boundary
        if (normalComponent > 0) {
            return BoundaryType::Divergent; // Plates moving apart
        } else {
            return BoundaryType::Convergent; // Plates moving together
        }
    } else {
        // Movement is primarily tangential to boundary
        return BoundaryType::Transform; // Plates sliding past each other
    }
}

float PlateGenerator::CalculateStressAtBoundary(const PlateBoundary& boundary,
                                              const TectonicPlate& plate1,
                                              const TectonicPlate& plate2)
{
    // Calculate average stress across all boundary points
    float totalStress = 0.0f;
    
    for (const auto& point : boundary.points) {
        // Calculate movement vectors at the boundary point
        glm::vec3 movement1 = plate1.CalculateMovementAt(point);
        glm::vec3 movement2 = plate2.CalculateMovementAt(point);
        
        // Calculate relative movement
        glm::vec3 relativeMovement = movement2 - movement1;
        
        // Stress is proportional to magnitude of relative movement
        float pointStress = glm::length(relativeMovement) * 100.0f; // Scale factor
        
        // Adjust stress based on boundary type
        switch (boundary.type) {
            case BoundaryType::Convergent:
                // Higher stress for convergent boundaries
                pointStress *= 1.5f;
                
                // Even higher if continental plates are colliding
                if (plate1.GetType() == PlateType::Continental && 
                    plate2.GetType() == PlateType::Continental) {
                    pointStress *= 2.0f;
                }
                break;
                
            case BoundaryType::Divergent:
                // Lower stress for divergent boundaries
                pointStress *= 0.8f;
                break;
                
            case BoundaryType::Transform:
                // Medium stress for transform boundaries
                pointStress *= 1.0f;
                break;
        }
        
        totalStress += pointStress;
    }
    
    // Return average stress
    return boundary.points.empty() ? 0.0f : totalStress / boundary.points.size();
}

float PlateGenerator::CalculateElevationAtPoint(const glm::vec3& point,
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

} // namespace Core
} // namespace WorldGen
