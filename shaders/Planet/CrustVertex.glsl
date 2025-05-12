#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 Color;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    // Apply all matrices in proper order - model must be used!
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = vec3(worldPos);
    
    // Force bright color for visibility
    Color = aColor;
    
    // Transform normal using normal matrix
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // Apply full MVP transformation
    gl_Position = projection * view * worldPos;
}