#pragma once

#include <string>
#include <map>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <glad/glad.h>

class FontRenderer {
public:
    FontRenderer();
    ~FontRenderer();

    /**
     * Initialize the font renderer
     * @return true if initialization was successful, false otherwise
     */
    bool initialize();

    /**
     * Render text at the specified position
     * @param text The string to render
     * @param position Top-left position of the text in screen space
     * @param scale Scaling factor for the text size (1.0f = original size)
     * @param color RGB color of the text (0-1 range)
     */
    void renderText(const std::string& text, const glm::vec2& position, float scale, const glm::vec3& color);

    /**
     * Set the projection matrix for text rendering
     * @param projection The projection matrix to use for transforming text coordinates
     */
    void setProjection(const glm::mat4& projection);

private:
    /**
     * Character information for font rendering
     */
    struct Character {
        unsigned int textureID;  // OpenGL texture ID for the character
        glm::ivec2 size;        // Size of the character glyph
        glm::ivec2 bearing;     // Offset from baseline to top-left of glyph
        unsigned int advance;   // Horizontal advance to next character
    };

    /**
     * Load a font file
     * @param fontPath Path to the font file
     * @return true if font was loaded successfully, false otherwise
     */
    bool loadFont(const std::string& fontPath);

    /**
     * Compile the shaders used for text rendering
     * @return true if shaders were compiled successfully, false otherwise
     */
    bool compileShaders();

    std::map<char, Character> characters;  // Map of loaded characters
    unsigned int shaderProgram;            // OpenGL shader program
    unsigned int VAO;                      // Vertex Array Object
    unsigned int VBO;                      // Vertex Buffer Object
    FT_Library library;                    // FreeType library instance
    FT_Face face;                          // FreeType font face
}; 