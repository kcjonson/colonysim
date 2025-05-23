#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include "../Generators/World.h"
#include "../../../Shader.h"

namespace WorldGen {
namespace Renderers {

/**
 * @brief Renders a World object on the screen.
 * 
 * This class provides visualization for the World object,
 * including options to visualize the mesh in different ways.
 */
class World {
public:    // Visualization options are now handled internally

    /**
     * @brief Construct a new World renderer.
     */
    World();
    
    /**
     * @brief Destroy the World renderer.
     */
    ~World();

    /**
     * @brief Set the world to render.
     * 
     * @param world Pointer to the world to render.
     */
    void SetWorld(const Generators::World* world);    /**
     * @brief Render the world.
     * 
     * @param viewMatrix The view matrix.
     * @param projectionMatrix The projection matrix.
     */
    void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * @brief Validate the geometry of all tiles before rendering.
     * 
     * Checks for issues like missing vertices, non-planar tiles, etc.
     */
    void ValidateTileGeometry();

    // Add getter for shader
    const Shader& getShader() const { return m_shader; }

private:    /**
     * @brief Generate rendering data for the world.
     */
    void GenerateRenderingData();
      struct TileFanInfo {
        unsigned int startIndex;    ///< Starting index in the vertex buffer
        unsigned int vertexCount;   ///< Number of vertices in this tile (center + perimeter)
        unsigned int indexCount;    ///< Number of indices for this tile's triangle fan
    };
    
    /**
     * @brief Render the world tiles.
     */
    void RenderTiles(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    
    /**
     * @brief Render a single tile.
     *
     * @param tileInfo The tile information for rendering.
     * @param isVisible Function to check if a vertex is visible.
     * @param fillMode Whether to render in fill mode (true) or wireframe mode (false).
     */
    void RenderTile(const TileFanInfo& tileInfo, 
                    const std::function<bool(const glm::vec3&)>& isVisible,
                    bool fillMode);
      const Generators::World* m_world;    ///< The world to render
    // OpenGL rendering data
    unsigned int m_vao;                  ///< Vertex array object
    unsigned int m_vbo;                  ///< Vertex buffer object
    unsigned int m_ebo;                  ///< Element buffer object
    Shader m_shader;                     ///< Shader object
    std::vector<TileFanInfo> m_tileFanInfo; ///< Triangle fan information for each tile

    // Render data for different modes    // Render data for different modes
    std::vector<float> m_vertexData;     ///< Vertex data for rendering
    std::vector<unsigned int> m_indices; ///< Indices for rendering
      bool m_dataGenerated;                ///< Whether rendering data has been generated
};

} // namespace Renderers
} // namespace WorldGen