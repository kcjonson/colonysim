#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>
#include "../../../Shader.h"
#include "../Generators/World.h"
#include "World.h" // Add the World renderer header

namespace WorldGen {
namespace Renderers {

// Constants for configuring the landing location indicator
namespace LandingLocationConstants {
    constexpr float DEFAULT_CIRCLE_RADIUS = 0.3f;    
    constexpr float INNER_RADIUS_RATIO = 0.8f;       // Inner circle is 80% of the outer radius (thinner ring)
    constexpr float OFFSET_FROM_SURFACE = 1.001f;    // Very small offset to prevent z-fighting
    constexpr float CIRCLE_SCALE_FACTOR = 0.05f;     
}

/**
 * @brief Renders a landing location indicator on the world surface.
 * 
 * This class draws a circular indicator on the planet surface
 * at a location determined by mouse position or a selected point.
 */
class LandingLocation {
public:
    /**
     * @brief Construct a new Landing Location renderer.
     * 
     * @param worldRenderer Reference to the world renderer for shader sharing.
     */
    LandingLocation(const World* worldRenderer);
    
    /**
     * @brief Destroy the Landing Location renderer.
     */
    ~LandingLocation();

    /**
     * @brief Set the world reference for the landing location.
     * 
     * @param world Pointer to the world to render on.
     */
    void SetWorld(const Generators::World* world);

    /**
     * @brief Render the landing location indicator.
     * 
     * @param viewMatrix The view matrix.
     * @param projectionMatrix The projection matrix.
     */
    void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * @brief Updates the landing location based on mouse position.
     * 
     * @param mouseX Screen space mouse X position.
     * @param mouseY Screen space mouse Y position.
     * @param viewMatrix The current view matrix.
     * @param projectionMatrix The current projection matrix.
     * @param windowWidth Window width.
     * @param windowHeight Window height.
     * @return true If the ray intersects with the planet.
     * @return false If the ray does not intersect with the planet.
     */
    bool UpdateFromMousePosition(float mouseX, float mouseY, 
                                 const glm::mat4& viewMatrix, 
                                 const glm::mat4& projectionMatrix,
                                 int windowWidth, int windowHeight);

    /**
     * @brief Handle mouse click to confirm landing location.
     * 
     * @return true If the location was selected.
     * @return false If no valid location could be selected.
     */
    bool SelectCurrentLocation();

    /**
     * @brief Check if a landing location has been selected.
     * 
     * @return true If a landing location has been selected.
     * @return false Otherwise.
     */
    bool HasLocationSelected() const { return m_locationSelected; }

    /**
     * @brief Get the selected landing location.
     * 
     * @return glm::vec3 The selected landing location.
     */
    glm::vec3 GetSelectedLocation() const { return m_selectedLocation; }
    
    /**
     * @brief Generate a dummy location for testing purposes.
     */
    void GenerateDummyLocation();

    /**
     * @brief Reset the landing location selection.
     */
    void Reset();

private:
    /**
     * @brief Generate the circle geometry for rendering.
     */
    void GenerateCircle();

    /**
     * @brief Ray-sphere intersection test.
     * 
     * @param rayOrigin Origin of the ray.
     * @param rayDirection Direction of the ray.
     * @param sphereCenter Center of the sphere.
     * @param sphereRadius Radius of the sphere.
     * @param intersectionPoint Output parameter for the intersection point.
     * @return true If the ray intersects with the sphere.
     * @return false If the ray does not intersect with the sphere.
     */
    bool RaySphereIntersect(const glm::vec3& rayOrigin, 
                            const glm::vec3& rayDirection,
                            const glm::vec3& sphereCenter, 
                            float sphereRadius,
                            glm::vec3& intersectionPoint);

    const Generators::World* m_world;           ///< The world to render on
    const World* m_worldRenderer;               ///< The world renderer for shader sharing
    unsigned int m_vao;                         ///< Vertex array object
    unsigned int m_vbo;                         ///< Vertex buffer object
    unsigned int m_ebo;                         ///< Element buffer object
    std::vector<float> m_vertexData;            ///< Vertex data for the circle
    std::vector<unsigned int> m_indices;        ///< Indices for the circle
    
    glm::vec3 m_currentLocation;                ///< Current indicator location
    glm::vec3 m_selectedLocation;               ///< Selected landing location
    bool m_locationSelected;                    ///< Whether a location has been selected
    float m_circleRadius;                       ///< Radius of the circle in world space
    int m_circleSections;                       ///< Number of sections in the circle
};

} // namespace Renderers
} // namespace WorldGen
