#pragma once
#include "Base.h"
#include "Border.h"

namespace Rendering {
namespace Styles {

// For shapes with borders (Rectangle, Circle)
class Rectangle : public Base, public Border {
public:
    Rectangle(const glm::vec4& color = glm::vec4(1.0f),
             float opacity = 1.0f,
             const glm::vec4& borderColor = glm::vec4(0.0f),
             float borderWidth = 0.0f,
             BorderPosition borderPosition = BorderPosition::Outside,
             float cornerRadius = 0.0f);
};

class Circle : public Base, public Border {
public:
    Circle(const glm::vec4& color = glm::vec4(1.0f),
          float opacity = 1.0f,
          const glm::vec4& borderColor = glm::vec4(0.0f),
          float borderWidth = 0.0f,
          BorderPosition borderPosition = BorderPosition::Outside);
};

// For shapes without borders (Line, Text)
class Line : public Base {
public:
    Line(const glm::vec4& color = glm::vec4(1.0f),
        float opacity = 1.0f);
};

class Text : public Base {
public:
    Text(const glm::vec4& color = glm::vec4(1.0f),
        float opacity = 1.0f);
};

class Polygon : public Base, public Border {
public:
    Polygon(const glm::vec4& color = glm::vec4(1.0f),
           float opacity = 1.0f,
           const glm::vec4& borderColor = glm::vec4(0.0f),
           float borderWidth = 0.0f,
           BorderPosition borderPosition = BorderPosition::Outside);
};

} // namespace Styles
} // namespace Rendering 