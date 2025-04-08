#version 330 core

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec4 color;
layout (location = 3) in vec4 gradientColor;
layout (location = 4) in float patternType;

out vec2 TexCoord;
out vec4 Color;
out vec4 GradientColor;
out float PatternType;
out vec3 WorldPos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    vec4 worldPos = model * vec4(position, 0.0, 1.0);
    WorldPos = worldPos.xyz;
    gl_Position = projection * view * worldPos;
    TexCoord = texCoord;
    Color = color;
    GradientColor = gradientColor;
    PatternType = patternType;
} 