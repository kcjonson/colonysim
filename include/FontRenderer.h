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

    bool initialize();
    void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color);
    void setProjection(const glm::mat4& projection);

private:
    struct Character {
        unsigned int textureID;
        glm::ivec2 size;
        glm::ivec2 bearing;
        unsigned int advance;
    };

    bool loadFont(const std::string& fontPath);
    bool compileShaders();

    std::map<char, Character> characters;
    unsigned int shaderProgram;
    unsigned int VAO;
    unsigned int VBO;
    FT_Library library;
    FT_Face face;
}; 