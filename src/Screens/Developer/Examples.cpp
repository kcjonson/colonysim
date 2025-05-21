#include "Examples.h"
#include <iostream>
#include <cmath> // For cos and sin

// Define PI as a constant
constexpr float PI = 3.14159265358979323846f;

// Examples constructor: Pass camera and window
Examples::Examples(Camera* cam, GLFWwindow* win) 
    : Rendering::Layer(0.0f, Rendering::ProjectionType::ScreenSpace, cam, win) {
    // Constructor body can be empty if initialization is done in the list
}

void Examples::initialize() {
    std::cout << "Initializing examples..." << std::endl;
    
    // Create examples for different shapes
    createRectangleExamples();
    createCircleExamples();
    createLineExamples();
    createPolygonExamples();
    createTextExamples();
    createTextInputExamples();
}

void Examples::createRectangleExamples() {
    
    // Example of rectangle with transparency
    auto transRect = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(250.0f, 250.0f),  // Top-left position
            .size = glm::vec2(100.0f, 100.0f),
            .style = Rendering::Styles::Rectangle({
                .color = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f) // Red with 50% transparency
            }),
            .zIndex = 10.0f  // Z-index
        }
    );
    addItem(transRect);

    // Example of rectangle with border and corner radius
    auto roundedRect = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(300.0f, 300.0f),  // Top-left position
            .size = glm::vec2(100.0f, 100.0f),
            .style = Rendering::Styles::Rectangle({
                .color = glm::vec4(0.0f, 0.0f, 1.0f, 0.3f),    // Blue with 30% transparency
                .borderColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // Red border
                .borderWidth = 2.0f,                           // 2px border width
                .borderPosition = BorderPosition::Outside,     // Border outside
                .cornerRadius = 20.0f                          // 20px corner radius
            }),
            .zIndex = 11.0f  // Z-index
        }
    );
    addItem(roundedRect);

    // Example of rectangle with green color
    auto greenRect = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(200.0f, 300.0f),  // Top-left position
            .size = glm::vec2(100.0f, 100.0f),
            .style = Rendering::Styles::Rectangle({
                .color = glm::vec4(0.0f, 1.0f, 0.0f, 0.7f)  // Green with 70% transparency
            }),
            .zIndex = 12.0f  // Z-index
        }
    );
    addItem(greenRect);

    // Example of rectangle with yellow border
    auto borderedRect = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(340.0f, 210.0f),  // Top-left position
            .size = glm::vec2(120.0f, 80.0f),
            .style = Rendering::Styles::Rectangle({
                .color = glm::vec4(0.0f, 0.0f, 1.0f, 0.4f),      // Blue with 40% transparency
                .borderColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // Yellow border
                .borderWidth = 10.0f,                            // Thick border
                .borderPosition = BorderPosition::Outside        // Border outside
            }),
            .zIndex = 13.0f  // Z-index
        }
    );
    addItem(borderedRect);

    // Example of rectangle with high corner radius (pill shape)
    auto pillRect = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(370.0f, 410.0f),  // Top-left position
            .size = glm::vec2(160.0f, 80.0f),
            .style = Rendering::Styles::Rectangle({
                .color = glm::vec4(1.0f, 0.5f, 0.0f, 0.8f),      // Orange with 80% transparency
                .borderColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), // Black border
                .borderWidth = 3.0f,                             // 3px border
                .borderPosition = BorderPosition::Inside,        // Inside border
                .cornerRadius = 40.0f                            // Very rounded corners
            }),
            .zIndex = 14.0f  // Z-index
        }
    );
    addItem(pillRect);
}

void Examples::createCircleExamples() {
    
    // Basic circle example - create style using constructor parameters
    Rendering::Styles::CircleStyleParams basicParams;
    basicParams.color = glm::vec4(1.0f, 0.0f, 0.0f, 0.7f); // Red with 70% transparency
    
    auto basicCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(100.0f, 300.0f), // Changed from -300.0f to 100.0f
        40.0f,  // Radius
        Rendering::Styles::Circle(basicParams),
        15.0f  // Z-index
    );
    addItem(basicCircle);
    
    // Circle with border
    Rendering::Styles::CircleStyleParams borderedParams;
    borderedParams.color = glm::vec4(0.0f, 0.0f, 1.0f, 0.5f);      // Blue with 50% transparency
    borderedParams.borderColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow border
    borderedParams.borderWidth = 5.0f;                              // 5px border
    borderedParams.borderPosition = BorderPosition::Outside;        // Outside border
    
    auto borderedCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(200.0f, 300.0f), // Changed from -200.0f to 200.0f
        35.0f,  // Radius
        Rendering::Styles::Circle(borderedParams),
        16.0f  // Z-index
    );
    addItem(borderedCircle);
    
    // Circle with inside border
    Rendering::Styles::CircleStyleParams insideBorderedParams;
    insideBorderedParams.color = glm::vec4(0.0f, 1.0f, 0.0f, 0.6f);      // Green with 60% transparency
    insideBorderedParams.borderColor = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // Purple border
    insideBorderedParams.borderWidth = 5.0f;                              // 5px border
    insideBorderedParams.borderPosition = BorderPosition::Inside;         // Inside border
    
    auto insideBorderedCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(300.0f, 300.0f), // Changed from -100.0f to 300.0f
        35.0f,  // Radius
        Rendering::Styles::Circle(insideBorderedParams),
        17.0f  // Z-index
    );
    addItem(insideBorderedCircle);
    
    // High segment circle for smoother appearance
    Rendering::Styles::CircleStyleParams smoothParams;
    smoothParams.color = glm::vec4(0.5f, 0.5f, 1.0f, 0.8f);      // Light blue with 80% transparency
    smoothParams.borderColor = glm::vec4(0.0f, 0.0f, 0.5f, 1.0f); // Dark blue border
    smoothParams.borderWidth = 2.0f;                              // 2px border
    smoothParams.borderPosition = BorderPosition::Center;         // Center border
    
    auto smoothCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(100.0f, 200.0f), // Changed from -300.0f to 100.0f
        50.0f,  // Radius
        Rendering::Styles::Circle(smoothParams),
        18.0f  // Z-index
    );
    addItem(smoothCircle);
    
    // Large transparent circle
    Rendering::Styles::CircleStyleParams largeParams;
    largeParams.color = glm::vec4(1.0f, 1.0f, 0.0f, 0.3f);      // Yellow with 30% transparency
    
    auto largeCircle = std::make_shared<Rendering::Shapes::Circle>(
        glm::vec2(250.0f, 150.0f), // Changed from -150.0f to 250.0f
        80.0f,  // Radius
        Rendering::Styles::Circle(largeParams),
        19.0f  // Z-index
    );
    addItem(largeCircle);
}

void Examples::createLineExamples() {
    // Basic line
    auto basicLine = std::make_shared<Rendering::Shapes::Line>(
        glm::vec2(100.0f, 100.0f), // Changed Y from 0.0f to 100.0f
        glm::vec2(300.0f, 100.0f), // Changed Y from 0.0f to 100.0f
        Rendering::Styles::Line({
            .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),  // Red
            .width = 2.0f                               // 2px width
        }),
        20.0f  // Z-index
    );
    addItem(basicLine);
    
    // Thick line
    auto thickLine = std::make_shared<Rendering::Shapes::Line>(
        glm::vec2(100.0f, 150.0f), // Changed Y from -50.0f to 150.0f
        glm::vec2(300.0f, 150.0f), // Changed Y from -50.0f to 150.0f
        Rendering::Styles::Line({
            .color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),  // Blue
            .width = 10.0f                              // 10px width
        }),
        21.0f  // Z-index
    );
    addItem(thickLine);
    
    // Diagonal line
    auto diagonalLine = std::make_shared<Rendering::Shapes::Line>(
        glm::vec2(100.0f, 200.0f), // Changed Y from -100.0f to 200.0f
        glm::vec2(300.0f, 250.0f), // Changed Y from -200.0f to 250.0f
        Rendering::Styles::Line({
            .color = glm::vec4(0.0f, 1.0f, 0.0f, 0.7f),  // Green with 70% transparency
            .width = 5.0f                               // 5px width
        }),
        22.0f  // Z-index
    );
    addItem(diagonalLine);
}

void Examples::createPolygonExamples() {
    // Triangle
    std::vector<glm::vec2> triangleVertices = {
        glm::vec2(0.0f, 50.0f),
        glm::vec2(-50.0f, -50.0f),
        glm::vec2(50.0f, -50.0f)
    };
    
    auto triangle = std::make_shared<Rendering::Shapes::Polygon>(
        glm::vec2(100.0f, 400.0f), // Changed Y from -300.0f to 400.0f
        triangleVertices,
        Rendering::Styles::Polygon({
            .color = glm::vec4(1.0f, 0.0f, 0.0f, 0.7f)  // Red with 70% transparency
        }),
        23.0f  // Z-index
    );
    addItem(triangle);
    
    // Pentagon
    std::vector<glm::vec2> pentagonVertices;
    for (int i = 0; i < 5; i++) {
        float angle = static_cast<float>(i) * 2.0f * PI / 5.0f;
        pentagonVertices.push_back(glm::vec2(50.0f * std::cos(angle), 50.0f * std::sin(angle)));
    }
    
    auto pentagon = std::make_shared<Rendering::Shapes::Polygon>(
        glm::vec2(250.0f, 400.0f), // Changed Y from -300.0f to 400.0f
        pentagonVertices,
        Rendering::Styles::Polygon({
            .color = glm::vec4(0.0f, 0.0f, 1.0f, 0.6f),      // Blue with 60% transparency
            .borderColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), // Black border
            .borderWidth = 2.0f,                              // 2px border
            .borderPosition = BorderPosition::Outside         // Outside border
        }),
        24.0f  // Z-index
    );
    addItem(pentagon);
    
    // Star
    std::vector<glm::vec2> starVertices;
    for (int i = 0; i < 10; i++) {
        float angle = static_cast<float>(i) * 2.0f * PI / 10.0f;
        float radius = (i % 2 == 0) ? 50.0f : 25.0f;  // Alternate between outer and inner radius
        starVertices.push_back(glm::vec2(radius * std::cos(angle), radius * std::sin(angle)));
    }
    
    auto star = std::make_shared<Rendering::Shapes::Polygon>(
        glm::vec2(400.0f, 400.0f), // Changed Y from -300.0f to 400.0f
        starVertices,
        Rendering::Styles::Polygon({
            .color = glm::vec4(1.0f, 1.0f, 0.0f, 0.8f),      // Yellow with 80% transparency
            .borderColor = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), // Orange border
            .borderWidth = 3.0f,                              // 3px border
            .borderPosition = BorderPosition::Inside          // Inside border
        }),
        25.0f  // Z-index
    );
    addItem(star);
}

void Examples::createTextExamples() {
    // Basic text
    auto basicText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Hello, World!",
            .position = glm::vec2(100.0f, 50.0f), 
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)  // Black
            }),
            .zIndex = 26.0f  // Z-index
        }
    );
    addItem(basicText);
    
    // Colored text
    auto coloredText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Colored Text",
            .position = glm::vec2(100.0f, 80.0f),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)  // Red
            }),
            .zIndex = 27.0f  // Z-index
        }
    );
    addItem(coloredText);
    
    // Semi-transparent text
    auto transparentText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Semi-transparent Text",
            .position = glm::vec2(100.0f, 110.0f),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(0.0f, 0.0f, 1.0f, 0.7f)  // Blue with 70% transparency
            }),
            .zIndex = 28.0f  // Z-index
        }
    );
    addItem(transparentText);
}

void Examples::createTextInputExamples() {
    // Label for the text input section
    auto textInputLabel = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Text Input Examples:",
            .position = glm::vec2(500.0f, 50.0f),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),  // Black
                .fontSize = 1.2f  // Slightly larger font
            }),
            .zIndex = 29.0f
        }
    );
    addItem(textInputLabel);
    
    // Basic text input example
    auto basicTextInput = std::make_shared<Rendering::Components::Form::Text>(
        Rendering::Components::Form::Text::Args{
            .label = "Basic Input:",
            .placeholder = "Type here...",
            .position = glm::vec2(500.0f, 80.0f),
            .size = glm::vec2(200.0f, 30.0f),
            .zIndex = 30.0f
        }
    );
    addItem(basicTextInput);
    
    // Styled text input example
    Rendering::Components::Form::Text::StyleParams customStyleParams;
    customStyleParams.color = glm::vec4(0.9f, 0.95f, 1.0f, 1.0f);  // Light blue background
    customStyleParams.borderColor = glm::vec4(0.4f, 0.6f, 0.9f, 1.0f);  // Blue border
    customStyleParams.focusColor = glm::vec4(0.85f, 0.9f, 1.0f, 1.0f);  // Lighter blue when focused
    customStyleParams.focusBorderColor = glm::vec4(0.2f, 0.4f, 0.8f, 1.0f);  // Darker blue border when focused
    customStyleParams.textColor = glm::vec4(0.1f, 0.1f, 0.5f, 1.0f);  // Dark blue text
    customStyleParams.placeholderColor = glm::vec4(0.5f, 0.6f, 0.7f, 1.0f);  // Gray-blue placeholder
    
    auto customStyledTextInput = std::make_shared<Rendering::Components::Form::Text>(
        Rendering::Components::Form::Text::Args{
            .label = "Styled Input:",
            .placeholder = "Custom styled input...",
            .position = glm::vec2(500.0f, 150.0f),
            .size = glm::vec2(250.0f, 35.0f),
            .style = Rendering::Components::Form::Text::Styles(customStyleParams),
            .zIndex = 31.0f,
            .onChange = [](const std::string& value) {
                // In a real application, you might do something with the value
                // For this example, we're just demonstrating the callback
            }
        }
    );
    addItem(customStyledTextInput);
    
    // Prefilled text input
    auto prefilledTextInput = std::make_shared<Rendering::Components::Form::Text>(
        Rendering::Components::Form::Text::Args{
            .label = "Prefilled Input:",
            .value = "Initial value",
            .position = glm::vec2(500.0f, 220.0f),
            .size = glm::vec2(200.0f, 30.0f),
            .zIndex = 32.0f
        }
    );
    addItem(prefilledTextInput);
    
    // Disabled text input
    auto disabledTextInput = std::make_shared<Rendering::Components::Form::Text>(
        Rendering::Components::Form::Text::Args{
            .label = "Disabled Input:",
            .value = "Cannot edit this",
            .position = glm::vec2(500.0f, 290.0f),
            .size = glm::vec2(200.0f, 30.0f),
            .disabled = true,
            .zIndex = 33.0f
        }
    );
    addItem(disabledTextInput);
}