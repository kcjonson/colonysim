#include "Examples.h"
#include <iostream>
#include <cmath> // For cos and sin

// Define PI as a constant
constexpr float PI = 3.14159265358979323846f;

Examples::Examples() : examplesLayer(std::make_shared<Rendering::Layer>(0.0f)) {
}

void Examples::initialize() {
    std::cout << "Initializing examples..." << std::endl;
    
    // Create examples for different shapes
    createRectangleExamples();
    createCircleExamples();
    createLineExamples();
    createPolygonExamples();
    createTextExamples();
}

void Examples::render(VectorGraphics& vectorGraphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    examplesLayer->render(vectorGraphics, viewMatrix, projectionMatrix);
}

void Examples::createRectangleExamples() {
    // Example of rectangle with transparency
    auto transRect = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(250.0f, 250.0f),  // Top-left position
        glm::vec2(100.0f, 100.0f),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f) // Red with 50% transparency
        }),
        10.0f  // Z-index
    );
    examplesLayer->addItem(transRect);
    
    // Example of rectangle with border and corner radius
    auto roundedRect = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(300.0f, 300.0f),  // Top-left position
        glm::vec2(100.0f, 100.0f),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.0f, 0.0f, 1.0f, 0.3f),    // Blue with 30% transparency
            .borderColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // Red border
            .borderWidth = 2.0f,                           // 2px border width
            .borderPosition = BorderPosition::Outside,     // Border outside
            .cornerRadius = 20.0f                          // 20px corner radius
        }),
        11.0f  // Z-index
    );
    examplesLayer->addItem(roundedRect);
    
    // Example of rectangle with green color
    auto greenRect = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(200.0f, 300.0f),  // Top-left position
        glm::vec2(100.0f, 100.0f),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.0f, 1.0f, 0.0f, 0.7f)  // Green with 70% transparency
        }),
        12.0f  // Z-index
    );
    examplesLayer->addItem(greenRect);
    
    // Example of rectangle with yellow border
    auto borderedRect = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(340.0f, 210.0f),  // Top-left position
        glm::vec2(120.0f, 80.0f),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.0f, 0.0f, 1.0f, 0.4f),      // Blue with 40% transparency
            .borderColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // Yellow border
            .borderWidth = 10.0f,                            // Thick border
            .borderPosition = BorderPosition::Outside        // Border outside
        }),
        13.0f  // Z-index
    );
    examplesLayer->addItem(borderedRect);
    
    // Example of rectangle with high corner radius (pill shape)
    auto pillRect = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(370.0f, 410.0f),  // Top-left position
        glm::vec2(160.0f, 80.0f),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(1.0f, 0.5f, 0.0f, 0.8f),      // Orange with 80% transparency
            .borderColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), // Black border
            .borderWidth = 3.0f,                             // 3px border
            .borderPosition = BorderPosition::Inside,        // Inside border
            .cornerRadius = 40.0f                            // Very rounded corners
        }),
        14.0f  // Z-index
    );
    examplesLayer->addItem(pillRect);
}

void Examples::createCircleExamples() {
    // Basic circle example - create style using constructor parameters
    Rendering::Styles::CircleStyleParams basicParams;
    basicParams.color = glm::vec4(1.0f, 0.0f, 0.0f, 0.7f); // Red with 70% transparency
    
    auto basicCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(-300.0f, 300.0f),
        40.0f,  // Radius
        Rendering::Styles::Circle(basicParams),
        15.0f  // Z-index
    );
    examplesLayer->addItem(basicCircle);
    
    // Circle with border
    Rendering::Styles::CircleStyleParams borderedParams;
    borderedParams.color = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);      // Blue with 50% transparency
    borderedParams.borderColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow border
    borderedParams.borderWidth = 5.0f;                              // 5px border
    borderedParams.borderPosition = BorderPosition::Outside;        // Outside border
    
    auto borderedCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(-200.0f, 300.0f),
        35.0f,  // Radius
        Rendering::Styles::Circle(borderedParams),
        16.0f  // Z-index
    );
    examplesLayer->addItem(borderedCircle);
    
    // Circle with inside border
    Rendering::Styles::CircleStyleParams insideBorderedParams;
    insideBorderedParams.color = glm::vec4(0.0f, 1.0f, 0.0f, 0.6f);      // Green with 60% transparency
    insideBorderedParams.borderColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // Purple border
    insideBorderedParams.borderWidth = 5.0f;                              // 5px border
    insideBorderedParams.borderPosition = BorderPosition::Inside;         // Inside border
    
    auto insideBorderedCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(-100.0f, 300.0f),
        35.0f,  // Radius
        Rendering::Styles::Circle(insideBorderedParams),
        17.0f  // Z-index
    );
    examplesLayer->addItem(insideBorderedCircle);
    
    // High segment circle for smoother appearance
    Rendering::Styles::CircleStyleParams smoothParams;
    smoothParams.color = glm::vec4(0.5f, 0.5f, 1.0f, 0.8f);      // Light blue with 80% transparency
    smoothParams.borderColor = glm::vec4(0.0f, 0.0f, 0.5f, 1.0f); // Dark blue border
    smoothParams.borderWidth = 2.0f;                              // 2px border
    smoothParams.borderPosition = BorderPosition::Center;         // Center border
    
    auto smoothCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(-300.0f, 200.0f),
        50.0f,  // Radius
        Rendering::Styles::Circle(smoothParams),
        18.0f  // Z-index
    );
    examplesLayer->addItem(smoothCircle);
    
    // Large transparent circle
    Rendering::Styles::CircleStyleParams largeParams;
    largeParams.color = glm::vec4(1.0f, 1.0f, 0.0f, 0.3f);      // Yellow with 30% transparency
    
    auto largeCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(-150.0f, 150.0f),
        80.0f,  // Radius
        Rendering::Styles::Circle(largeParams),
        19.0f  // Z-index
    );
    examplesLayer->addItem(largeCircle);
}

void Examples::createLineExamples() {
    // Basic line
    auto basicLine = std::make_shared<Rendering::Shapes::Line>(
        glm::vec2(-300.0f, 0.0f),
        glm::vec2(-100.0f, 0.0f),
        Rendering::Styles::Line({
            .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),  // Red
            .width = 2.0f                               // 2px width
        }),
        20.0f  // Z-index
    );
    examplesLayer->addItem(basicLine);
    
    // Thick line
    auto thickLine = std::make_shared<Rendering::Shapes::Line>(
        glm::vec2(-300.0f, -50.0f),
        glm::vec2(-100.0f, -50.0f),
        Rendering::Styles::Line({
            .color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),  // Blue
            .width = 10.0f                              // 10px width
        }),
        21.0f  // Z-index
    );
    examplesLayer->addItem(thickLine);
    
    // Diagonal line
    auto diagonalLine = std::make_shared<Rendering::Shapes::Line>(
        glm::vec2(-300.0f, -100.0f),
        glm::vec2(-100.0f, -200.0f),
        Rendering::Styles::Line({
            .color = glm::vec4(0.0f, 1.0f, 0.0f, 0.7f),  // Green with 70% transparency
            .width = 5.0f                               // 5px width
        }),
        22.0f  // Z-index
    );
    examplesLayer->addItem(diagonalLine);
}

void Examples::createPolygonExamples() {
    // Triangle
    std::vector<glm::vec2> triangleVertices = {
        glm::vec2(0.0f, 50.0f),
        glm::vec2(-50.0f, -50.0f),
        glm::vec2(50.0f, -50.0f)
    };
    
    auto triangle = std::make_shared<Rendering::Shapes::Polygon>(
        glm::vec2(-300.0f, -300.0f),
        triangleVertices,
        Rendering::Styles::Polygon({
            .color = glm::vec4(1.0f, 0.0f, 0.0f, 0.7f)  // Red with 70% transparency
        }),
        23.0f  // Z-index
    );
    examplesLayer->addItem(triangle);
    
    // Pentagon
    std::vector<glm::vec2> pentagonVertices;
    for (int i = 0; i < 5; i++) {
        float angle = static_cast<float>(i) * 2.0f * PI / 5.0f;
        pentagonVertices.push_back(glm::vec2(50.0f * std::cos(angle), 50.0f * std::sin(angle)));
    }
    
    auto pentagon = std::make_shared<Rendering::Shapes::Polygon>(
        glm::vec2(-150.0f, -300.0f),
        pentagonVertices,
        Rendering::Styles::Polygon({
            .color = glm::vec4(0.0f, 0.0f, 1.0f, 0.6f),      // Blue with 60% transparency
            .borderColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), // Black border
            .borderWidth = 2.0f,                              // 2px border
            .borderPosition = BorderPosition::Outside         // Outside border
        }),
        24.0f  // Z-index
    );
    examplesLayer->addItem(pentagon);
    
    // Star
    std::vector<glm::vec2> starVertices;
    for (int i = 0; i < 10; i++) {
        float angle = static_cast<float>(i) * 2.0f * PI / 10.0f;
        float radius = (i % 2 == 0) ? 50.0f : 25.0f;  // Alternate between outer and inner radius
        starVertices.push_back(glm::vec2(radius * std::cos(angle), radius * std::sin(angle)));
    }
    
    auto star = std::make_shared<Rendering::Shapes::Polygon>(
        glm::vec2(0.0f, -300.0f),
        starVertices,
        Rendering::Styles::Polygon({
            .color = glm::vec4(1.0f, 1.0f, 0.0f, 0.8f),      // Yellow with 80% transparency
            .borderColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), // Orange border
            .borderWidth = 3.0f,                              // 3px border
            .borderPosition = BorderPosition::Inside          // Inside border
        }),
        25.0f  // Z-index
    );
    examplesLayer->addItem(star);
}

void Examples::createTextExamples() {
    // Basic text
    auto basicText = std::make_shared<Rendering::Shapes::Text>(
        "Hello, World!",
        glm::vec2(100.0f, 0.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)  // Black
        }),
        26.0f  // Z-index
    );
    examplesLayer->addItem(basicText);
    
    // Colored text
    auto coloredText = std::make_shared<Rendering::Shapes::Text>(
        "Colored Text",
        glm::vec2(100.0f, -50.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)  // Red
        }),
        27.0f  // Z-index
    );
    examplesLayer->addItem(coloredText);
    
    // Semi-transparent text
    auto transparentText = std::make_shared<Rendering::Shapes::Text>(
        "Semi-transparent Text",
        glm::vec2(100.0f, -100.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(0.0f, 0.0f, 1.0f, 0.7f)  // Blue with 70% transparency
        }),
        28.0f  // Z-index
    );
    examplesLayer->addItem(transparentText);
} 