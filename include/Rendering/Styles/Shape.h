#pragma once
#include "Base.h"
#include "Border.h"

namespace Rendering {
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
};

struct TextStyleParams {
    glm::vec4 color = glm::vec4(1.0f);
    float opacity = 1.0f;
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
};

class Text : public Base {
public:
    // Only use named parameters
    Text(const TextStyleParams& params = {});
};

class Polygon : public Base, public Border {
public:
    // Only use named parameters
    Polygon(const PolygonStyleParams& params = {});
};

} // namespace Styles
} // namespace Rendering 