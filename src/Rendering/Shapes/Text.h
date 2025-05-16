#pragma once

#include "Rendering/Shapes/Shape.h"
#include "Rendering/Styles/Shape.h"
#include <glm/glm.hpp>
#include <string>

namespace Rendering {
namespace Shapes {

class Text : public Shape {
public:    /**
     * Text styles namespace with all text related styling
     */
    class Styles : public Rendering::Styles::Base {
    public:
        /**
         * Parameters for text styling
         */
        struct StyleParams {
            glm::vec4 color = glm::vec4(1.0f);
            float opacity = 1.0f;
            float fontSize = 16.0f;  // Default font size
            TextAlign::Horizontal horizontalAlign = TextAlign::Horizontal::Left;
            TextAlign::Vertical verticalAlign = TextAlign::Vertical::Top;
            glm::vec2 size = glm::vec2(0.0f);  // Size of the text box (width and height) - zero means auto-size
        };

        /**
         * Text style constructor
         */
        Styles(const StyleParams& params = {});
        virtual ~Styles() = default;

        // Style properties
        glm::vec4 color;
        float opacity;
        float fontSize;
        TextAlign::Horizontal horizontalAlign;
        TextAlign::Vertical verticalAlign;
        glm::vec2 size; // Size of the text box (width and height)
    };
    
    /**
     * Arguments struct for Text constructor
     */
    struct Args {
        std::string text = "";
        glm::vec2 position = glm::vec2(0.0f);
        Styles style = Styles({});
        float zIndex = 0.0f;
    };
    
    /**
     * Create a text shape using the Args struct.
     * 
     * @param args A struct containing all arguments for the text.
     */
    explicit Text(const Args& args);
  
    
    virtual ~Text() = default;
    
    // Getters and setters
    const std::string& getText() const { return text; }
    void setText(const std::string& t) { text = t; markDirty(); }

    const Styles& getStyle() const { return style; }
    void setStyle(const Styles& s) { style = s; markDirty(); }
    
    const glm::vec2& getSize() const { return style.size; }
    void setSize(const glm::vec2& s) { style.size = s; markDirty(); }    // Implementation of the draw method
    virtual void draw() override;

private:
    std::string text;
    Styles style;
};

} // namespace Shapes
} // namespace Rendering