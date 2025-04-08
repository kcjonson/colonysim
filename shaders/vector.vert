#version 330 core
layout (location = 0) in vec2 position;
layout (location = 2) in vec4 color;
out vec4 Color;
uniform mat4 projection;
uniform mat4 view;
void main() {
    gl_Position = projection * view * vec4(position, 0.0, 1.0);
    Color = color;
}