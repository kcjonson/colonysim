#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader {
public:
    Shader() : program(0) {}
    ~Shader() { if (program) glDeleteProgram(program); }
    
    bool loadFromFile(const char* vertexPath, const char* fragmentPath);
    void use() const { glUseProgram(program); }
    void setUniform(const char* name, const glm::mat4& value) const;
    
    GLuint getProgram() const { return program; }
    
private:
    GLuint program;
}; 