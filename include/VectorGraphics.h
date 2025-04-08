#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct Vertex {
    glm::vec2 position;
    glm::vec4 color;
};

class Shader {
public:
    Shader() : program(0) {}
    ~Shader() { if (program) glDeleteProgram(program); }
    
    bool loadFromFile(const char* vertexPath, const char* fragmentPath);
    void use() const { glUseProgram(program); }
    void setUniform(const char* name, const glm::mat4& value) const;
    
private:
    GLuint program;
};

class VectorGraphics {
public:
    VectorGraphics();
    ~VectorGraphics();

    // Initialize OpenGL resources after GLAD is initialized
    bool initialize();

    void render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    void clear();

    void drawRectangle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void drawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments = 32);
    void drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float width = 1.0f);
    void drawPolygon(const std::vector<glm::vec2>& points, const glm::vec4& color);

private:
    void updateBuffers();

    Shader shader;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    bool initialized;
}; 