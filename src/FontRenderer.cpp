#include "FontRenderer.h"
#include <iostream>
#include <fstream>
#include <sstream>

FontRenderer::FontRenderer() : shaderProgram(0), VAO(0), VBO(0), library(nullptr) {
    std::cout << "FontRenderer constructor called" << std::endl;
}

FontRenderer::~FontRenderer() {
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
    }
    if (library) {
        FT_Done_FreeType(library);
    }
}

bool FontRenderer::initialize() {
    std::cout << "Initializing FontRenderer..." << std::endl;
    
    // Initialize FreeType
    if (FT_Init_FreeType(&library)) {
        std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return false;
    }
    std::cout << "FreeType initialized successfully" << std::endl;

    // Load font
    if (!loadFont("fonts/Roboto-Regular.ttf")) {
        std::cerr << "Failed to load font" << std::endl;
        return false;
    }
    std::cout << "Font loaded successfully" << std::endl;

    // Compile shaders
    if (!compileShaders()) {
        std::cerr << "Failed to compile shaders" << std::endl;
        return false;
    }
    std::cout << "Shaders compiled successfully" << std::endl;

    // Setup buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::cout << "FontRenderer initialization complete" << std::endl;
    return true;
}

bool FontRenderer::loadFont(const std::string& fontPath) {
    std::cout << "Loading font from: " << fontPath << std::endl;
    
    if (FT_New_Face(library, fontPath.c_str(), 0, &face)) {
        std::cerr << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);
    std::cout << "Font face loaded successfully" << std::endl;

    // Load first 128 characters of ASCII set
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "ERROR::FREETYPE: Failed to load Glyph" << std::endl;
            continue;
        }

        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x)
        };
        characters[c] = character;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);

    std::cout << "Loaded " << characters.size() << " characters" << std::endl;
    return true;
}

bool FontRenderer::compileShaders() {
    std::cout << "Compiling shaders..." << std::endl;
    
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec4 vertex;
        out vec2 TexCoords;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoords;
        out vec4 color;
        uniform sampler2D text;
        uniform vec3 textColor;
        void main() {
            vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
            color = vec4(textColor, 1.0) * sampled;
        }
    )";

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::cout << "Shaders compiled and linked successfully" << std::endl;
    return true;
}

void FontRenderer::renderText(const std::string& text, const glm::vec2& position, float scale, const glm::vec3& color) {
    // Enable blending for text transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set up shader and uniforms
    glUseProgram(shaderProgram);
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Create a local copy of the position to avoid modifying the const reference
    glm::vec2 currentPos = position;

    // Iterate through each character in the text
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        Character ch = characters[*c];

        // Calculate position of character quad
        // xpos: current position + horizontal bearing
        // ypos: current position + vertical bearing (to align along baseline)
        float xpos = currentPos.x + ch.bearing.x * scale;
        float ypos = currentPos.y + ch.bearing.y * scale;

        // Calculate width and height of character quad
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;

        // Define vertices for character quad
        // Each vertex contains: x, y, texture_x, texture_y
        // Texture coordinates are flipped vertically (0.0f at bottom, 1.0f at top)
        // to match OpenGL's texture coordinate system while keeping the text upright
        float vertices[6][4] = {
            // First triangle
            { xpos,     ypos - h,   0.0f, 0.0f },  // Top-left
            { xpos,     ypos,       0.0f, 1.0f },  // Bottom-left
            { xpos + w, ypos,       1.0f, 1.0f },  // Bottom-right

            // Second triangle
            { xpos,     ypos - h,   0.0f, 0.0f },  // Top-left
            { xpos + w, ypos,       1.0f, 1.0f },  // Bottom-right
            { xpos + w, ypos - h,   1.0f, 0.0f }   // Top-right
        };

        // Render character quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Advance cursor position for next character
        // Advance is in 1/64 pixels, so we need to divide by 64
        currentPos.x += (ch.advance >> 6) * scale;
    }

    // Clean up OpenGL state
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

void FontRenderer::setProjection(const glm::mat4& projection) {
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
} 