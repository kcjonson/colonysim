#include "LandingLocation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>

namespace WorldGen {
namespace Renderers {

LandingLocation::LandingLocation(const World* worldRenderer)    : world(nullptr)
    , worldRenderer(worldRenderer)
    , vao(0)
    , vbo(0)
    , ebo(0)    , locationSelected(false)
    , circleRadius(LandingLocationConstants::DEFAULT_CIRCLE_RADIUS)  // Use the constant from header
    , circleSections(32)  // Number of sections in the circle
{
    // Initialize with empty locations
    currentLocation = glm::vec3(0.0f);
    selectedLocation = glm::vec3(0.0f);
      // Initialize the circle indicator
    GenerateCircle();
}

LandingLocation::~LandingLocation()
{
    // Clean up OpenGL resources
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
    
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
    }
    
    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
    }
}

void LandingLocation::SetWorld(const Generators::World* world)
{
    this->world = world;
}

void LandingLocation::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (!world || !worldRenderer) {
        std::cerr << "Cannot render landing location: world or world renderer not initialized" << std::endl;
        return;
    }
    
    // Only render if we have a current location to hover over or a selected location
    if (!locationSelected && glm::length(currentLocation) < 0.001f) {
        // No location to render
        return;
    }
    
    // Save current OpenGL state
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
      // Setup for maximum visibility
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL); // Use LEQUAL instead of ALWAYS so it respects depth but doesn't get completely hidden
    
    // Enable alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Disable face culling
    glDisable(GL_CULL_FACE);
    
    // Use the world renderer's shader
    GLuint shaderProgram = worldRenderer->getShader().getProgram();
    glUseProgram(shaderProgram);
      // Get world radius but don't trust it - use fixed value if invalid
    float worldRadius = world->GetRadius();
    
    // Force a fixed scale since world radius is incorrect
    // Most planet renderers use a scale of 1.0 for unit spheres
    float scale = 1.0f;
    
    // Use either the hovering location or the selected location
    glm::vec3 location = locationSelected ? selectedLocation : currentLocation;
    
    // Calculate the rotation to align the circle with the surface normal
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 normal = glm::normalize(location);
    
    // Create a rotation matrix to orient the circle on the sphere surface
    glm::vec3 right = glm::cross(up, normal);
    if (glm::length(right) < 0.001f) {
        // If normal is parallel to up, use a different reference vector
        right = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), normal);
    }
    right = glm::normalize(right);
    glm::vec3 forward = glm::normalize(glm::cross(normal, right));
    
    glm::mat4 rotationMatrix(1.0f);
    rotationMatrix[0] = glm::vec4(right, 0.0f);
    rotationMatrix[1] = glm::vec4(normal, 0.0f);
    rotationMatrix[2] = glm::vec4(forward, 0.0f);    // Position the circle at the target location on the sphere
    // Use a slightly larger radius to avoid z-fighting with the planet surface
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), location * LandingLocationConstants::OFFSET_FROM_SURFACE);
      // Apply appropriate scaling for the circle indicator based on constants
    float circleScale = LandingLocationConstants::CIRCLE_SCALE_FACTOR; // Use smaller scale for better precision
    glm::mat4 modelMatrix = translationMatrix * rotationMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(circleScale));
    
    // Set the model matrix uniform
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    
    // Set the view and projection matrix uniforms
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    
    // Force shader to use vertex colors 
    GLint useColorAttribLoc = glGetUniformLocation(shaderProgram, "useColorAttrib");
    if (useColorAttribLoc != -1) {
        glUniform1i(useColorAttribLoc, 1); // Set to 1 to use vertex colors
    }
    
    // Draw the circle
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    
    // Reset shader to use planetColor if needed
    if (useColorAttribLoc != -1) {
        glUniform1i(useColorAttribLoc, 0); // Reset to default
    }
    
    // Restore depth function
    glDepthFunc(GL_LEQUAL);
    
    // Unuse shader
    glUseProgram(0);
    
    // Restore previous OpenGL state
    if (cullFaceEnabled) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS); // Reset to default depth function
    }
    else glDisable(GL_DEPTH_TEST);
    
    if (blendEnabled) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
}

bool LandingLocation::UpdateFromMousePosition(float mouseX, float mouseY, 
                                             const glm::mat4& viewMatrix, 
                                             const glm::mat4& projectionMatrix,
                                             int windowWidth, int windowHeight)
{
    // If world is not set or location already selected, don't update
    if (!world || locationSelected) {
        return false;
    }
    
    // Convert mouse coordinates to normalized device coordinates (NDC)
    // The mouseX and mouseY are relative to the viewport (already adjusted for sidebar)
    float ndcX = (2.0f * mouseX) / windowWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / windowHeight; // Flip Y: top is 1, bottom is -1
      // Create a ray starting at the near plane going through the clicked point
    glm::vec4 rayStart = glm::vec4(ndcX, ndcY, -1.0f, 1.0f); // Near plane
    glm::vec4 rayEnd = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);    // Far plane
    
    // Convert to eye space
    glm::mat4 invProjection = glm::inverse(projectionMatrix);
    glm::vec4 rayStartEye = invProjection * rayStart;
    glm::vec4 rayEndEye = invProjection * rayEnd;
    
    // Perspective division to get the actual points in eye space
    rayStartEye /= rayStartEye.w;
    rayEndEye /= rayEndEye.w;
    
    // Convert to world space
    glm::mat4 invView = glm::inverse(viewMatrix);
    glm::vec4 rayStartWorld = invView * rayStartEye;
    glm::vec4 rayEndWorld = invView * rayEndEye;
    
    // Calculate ray origin and direction
    glm::vec3 rayOrigin = glm::vec3(rayStartWorld);
    glm::vec3 rayDirection = glm::normalize(glm::vec3(rayEndWorld) - rayOrigin);
    
    // World center is at origin
    glm::vec3 worldCenter(0.0f);
    
    // Always use a fixed radius of 1.0 for ray-sphere intersection 
    // since the planet appears to be a unit sphere
    float worldRadius = 1.0f;    // Test for intersection
    glm::vec3 intersectionPoint;
    bool hit = RaySphereIntersect(rayOrigin, rayDirection, worldCenter, worldRadius, intersectionPoint);
    
    if (hit) {
        // Update current location to the intersection point (normalized to surface)
        currentLocation = glm::normalize(intersectionPoint);
        
        // Regenerate the circle with appropriate colors
        GenerateCircle();
                  
        return true;
    }
    
    // No intersection, reset current location
    currentLocation = glm::vec3(0.0f);
    return false;
}

bool LandingLocation::SelectCurrentLocation()
{
    if (!world) {
        std::cerr << "Cannot select landing location: no world set" << std::endl;
        return false;
    }
    
    // Check if we have a valid location to select
    float locationLength = glm::length(currentLocation);
    std::cout << "Current location length: " << locationLength << std::endl;
    
    if (locationLength < 0.001f) {
        std::cerr << "Cannot select landing location: no valid location (length < 0.001)" << std::endl;
        return false;
    }
    
    // Store the selected location
    selectedLocation = currentLocation;
    locationSelected = true;
    
    // Regenerate the circle with the selected color
    GenerateCircle();
    
    // Log selection for debugging
    std::cout << "User has selected landing site at: [" 
              << selectedLocation.x << ", " 
              << selectedLocation.y << ", " 
              << selectedLocation.z << "]" << std::endl;
    
    return true;
}

void LandingLocation::Reset()
{
    locationSelected = false;
    currentLocation = glm::vec3(0.0f);
    selectedLocation = glm::vec3(0.0f);
    
    // Regenerate the circle to remove the selected color
    GenerateCircle();
}

void LandingLocation::GenerateCircle()
{
    // Clear previous data
    vertexData.clear();
    indices.clear();
      // Create a flat circle in the XZ plane (Y is up)
    const float innerRadius = circleRadius * LandingLocationConstants::INNER_RADIUS_RATIO; // Inner circle for ring effect
      // Define colors for better visibility - much brighter
    glm::vec3 circleColor;
    
    if (locationSelected) {
        // Bright red for selected - very visible
        circleColor = glm::vec3(1.0f, 0.0f, 0.0f); // Bright red
    } else {
        // Extremely bright color for hovering - visible against any background
        circleColor = glm::vec3(1.0f, 0.7f, 0.0f); // Bright amber/orange color
    }
    
    // Center vertex
    vertexData.push_back(0.0f); // x
    vertexData.push_back(0.0f); // y
    vertexData.push_back(0.0f); // z
    vertexData.push_back(0.0f); // normal x
    vertexData.push_back(1.0f); // normal y (pointing up)
    vertexData.push_back(0.0f); // normal z
    vertexData.push_back(circleColor.r); // color r (using color instead of texture coordinates)
    vertexData.push_back(circleColor.g); // color g
    vertexData.push_back(circleColor.b); // color b
    
    // Generate vertices for the outer circle
    for (int i = 0; i < circleSections; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / circleSections;
        float x = circleRadius * cos(angle);
        float z = circleRadius * sin(angle);
        
        // Outer circle vertex
        vertexData.push_back(x);
        vertexData.push_back(0.0f);
        vertexData.push_back(z);
        vertexData.push_back(0.0f);
        vertexData.push_back(1.0f);
        vertexData.push_back(0.0f);
        vertexData.push_back(circleColor.r);
        vertexData.push_back(circleColor.g);
        vertexData.push_back(circleColor.b);
        
        // Inner circle vertex (to create a ring)
        x = innerRadius * cos(angle);
        z = innerRadius * sin(angle);
        
        vertexData.push_back(x);
        vertexData.push_back(0.0f);
        vertexData.push_back(z);
        vertexData.push_back(0.0f);
        vertexData.push_back(1.0f);
        vertexData.push_back(0.0f);
        vertexData.push_back(circleColor.r);
        vertexData.push_back(circleColor.g);
        vertexData.push_back(circleColor.b);
    }
    
    // Create indices for triangles
    for (int i = 0; i < circleSections; ++i) {
        int outerIdx = 1 + i * 2;
        int innerIdx = 2 + i * 2;
        int nextOuterIdx = 1 + ((i + 1) % circleSections) * 2;
        int nextInnerIdx = 2 + ((i + 1) % circleSections) * 2;
        
        // First triangle of the sector
        indices.push_back(innerIdx);
        indices.push_back(outerIdx);
        indices.push_back(nextOuterIdx);
        
        // Second triangle of the sector
        indices.push_back(innerIdx);
        indices.push_back(nextOuterIdx);
        indices.push_back(nextInnerIdx);
    }
    
    // Set up OpenGL objects
    // Create and bind VAO
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);
    
    // Create and bind VBO
    if (vbo == 0) {
        glGenBuffers(1, &vbo);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
    
    // Create and bind EBO
    if (ebo == 0) {
        glGenBuffers(1, &ebo);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Color attribute (replaces texture coordinates)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
      // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

bool LandingLocation::RaySphereIntersect(const glm::vec3& rayOrigin, 
                                         const glm::vec3& rayDirection,
                                         const glm::vec3& sphereCenter, 
                                         float sphereRadius,
                                         glm::vec3& intersectionPoint)
{
    // Validate radius
    if (sphereRadius <= 0.0f) {
        std::cerr << "Invalid sphere radius in ray-sphere intersection: " << sphereRadius << std::endl;
        return false;
    }

    // Ensure ray direction is normalized
    glm::vec3 dir = glm::normalize(rayDirection);
    
    // Ray-sphere intersection formula 
    // Vector from ray origin to sphere center
    glm::vec3 oc = rayOrigin - sphereCenter;
    
    // Calculate quadratic formula coefficients
    float a = 1.0f; // dot(dir, dir) = 1 since dir is normalized
    float b = 2.0f * glm::dot(oc, dir);
    float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
    
    // Calculate discriminant
    float discriminant = b * b - 4.0f * a * c;
    
    if (discriminant < 0.0f) {
        // No intersection
        return false;
    }
    
    // Find nearest intersection (smallest positive t)
    float sqrtDisc = std::sqrt(discriminant);
    float t1 = (-b - sqrtDisc) / (2.0f * a);
    float t2 = (-b + sqrtDisc) / (2.0f * a);
    
    // Choose the closer intersection point that's in front of the ray
    float t;
    if (t1 > 0.001f) { // Small epsilon to avoid precision issues
        t = t1; // Front face hit
    }
    else if (t2 > 0.001f) {
        t = t2; // Ray starts inside sphere
    }
    else {
        // Both intersections behind the ray or too close
        return false;
    }
    
    // Calculate the intersection point
    intersectionPoint = rayOrigin + t * dir;
    
    return true;
}

void LandingLocation::GenerateDummyLocation()
{
    // Create a fixed test location on the equator
    currentLocation = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
    
    // If we want to immediately select it
    selectedLocation = currentLocation;
    locationSelected = true;
    
    // Generate the circle geometry with appropriate colors
    GenerateCircle();
    
    std::cout << "Generated dummy location at: [" 
              << currentLocation.x << ", " 
              << currentLocation.y << ", " 
              << currentLocation.z << "]" << std::endl;
}

} // namespace Renderers
} // namespace WorldGen
