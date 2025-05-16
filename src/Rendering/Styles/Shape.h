#pragma once
#include "Base.h"
#include "Border.h"

namespace Rendering {

// Text alignment enumerations
namespace TextAlign {
    enum Horizontal {
        Left,
        Center,
        Right
    };

    enum Vertical {
        Top,
        Middle,
        Bottom
    };
}

namespace Styles {

// Parameter structs for named initialization
struct RectangleStyleParams {
    glm::vec4 color = glm::vec4(1.0f);
    float opacity = 1.0f;
    glm::vec4 borderColor = glm::vec4(0.0f);
    float borderWidth = 0.0f;
    BorderPosition borderPosition = BorderPosition::Outside;
    float cornerRadius = 0.0f;
};

struct CircleStyleParams {
    glm::vec4 color = glm::vec4(1.0f);
    float opacity = 1.0f;
    glm::vec4 borderColor = glm::vec4(0.0f);
    float borderWidth = 0.0f;
    BorderPosition borderPosition = BorderPosition::Outside;
};

struct LineStyleParams {
    glm::vec4 color = glm::vec4(1.0f);
    float opacity = 1.0f;
    float width = 1.0f;  // Width of the line
};

struct TextStyleParams {
    glm::vec4 color = glm::vec4(1.0f);
    float opacity = 1.0f;
    float fontSize = 16.0f;  // Default font size
    TextAlign::Horizontal horizontalAlign = TextAlign::Horizontal::Left;
    TextAlign::Vertical verticalAlign = TextAlign::Vertical::Top;
    glm::vec2 size = glm::vec2(0.0f);  // Size of the text box (width and height) - zero means auto-size
};

struct PolygonStyleParams {
    glm::vec4 color = glm::vec4(1.0f);
    float opacity = 1.0f;
    glm::vec4 borderColor = glm::vec4(0.0f);
    float borderWidth = 0.0f;
    BorderPosition borderPosition = BorderPosition::Outside;
};

// For shapes with borders (Rectangle, Circle)
class Rectangle : public Base, public Border {
public:
    // Only use named parameters
    Rectangle(const RectangleStyleParams& params = {});
};

class Circle : public Base, public Border {
public:
    // Only use named parameters
    Circle(const CircleStyleParams& params = {});
};

// For shapes without borders (Line, Text)
class Line : public Base {
public:
    // Only use named parameters
    Line(const LineStyleParams& params = {});
    
    float width;  // Width of the line
};

class Text : public Base {
public:
    // Only use named parameters
    Text(const TextStyleParams& params = {});
    
    // Text specific properties
    float fontSize;
    TextAlign::Horizontal horizontalAlign;
    TextAlign::Vertical verticalAlign;
    glm::vec2 size; // Size of the text box (width and height)
};

class Polygon : public Base, public Border {
public:
    // Only use named parameters
    Polygon(const PolygonStyleParams& params = {});
};

} // namespace Styles
} // namespace Rendering