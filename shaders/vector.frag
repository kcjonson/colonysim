#version 330 core

in vec2 TexCoord;
in vec4 Color;
in vec4 GradientColor;
in float PatternType;
in vec3 WorldPos;

out vec4 FragColor;

uniform sampler2D texture0;
uniform float thickness;
uniform vec2 viewportSize;
uniform float time;

// Pattern types
const float PATTERN_NONE = 0.0;
const float PATTERN_GRID = 1.0;
const float PATTERN_DOTS = 2.0;
const float PATTERN_LINES = 3.0;

float gridPattern(vec2 uv, float scale) {
    vec2 grid = abs(fract(uv * scale - 0.5) - 0.5) / fwidth(uv * scale);
    float line = min(grid.x, grid.y);
    return 1.0 - min(line, 1.0);
}

float dotPattern(vec2 uv, float scale) {
    vec2 grid = fract(uv * scale);
    float dist = length(grid - 0.5);
    return smoothstep(0.3, 0.2, dist);
}

float linePattern(vec2 uv, float scale) {
    float lines = abs(sin(uv.y * scale * 3.14159));
    return smoothstep(0.8, 0.9, lines);
}

void main() {
    // Calculate distance from center for smooth edges
    vec2 center = vec2(0.5, 0.5);
    float dist = length(TexCoord - center);
    
    // Smooth step for anti-aliasing
    float alpha = smoothstep(0.5 - thickness, 0.5 + thickness, 1.0 - dist);
    
    // Gradient
    vec4 finalColor = mix(Color, GradientColor, TexCoord.y);
    
    // Pattern
    float pattern = 0.0;
    if (PatternType == PATTERN_GRID) {
        pattern = gridPattern(WorldPos.xy, 10.0);
    } else if (PatternType == PATTERN_DOTS) {
        pattern = dotPattern(WorldPos.xy, 10.0);
    } else if (PatternType == PATTERN_LINES) {
        pattern = linePattern(WorldPos.xy, 10.0);
    }
    
    // Combine color with pattern and alpha
    finalColor = mix(finalColor, vec4(1.0), pattern * 0.2);
    FragColor = finalColor * vec4(1.0, 1.0, 1.0, alpha);
} 