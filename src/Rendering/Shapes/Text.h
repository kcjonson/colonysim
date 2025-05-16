#pragma once

#include "Rendering/Shapes/Shape.h"
#include "Rendering/Styles/Shape.h"
#include <glm/glm.hpp>
#include <string>

namespace Rendering {
    namespace Shapes {

        class Text : public Shape {

            public:
                struct Styles {
                    glm::vec4 color = glm::vec4(1.0f);
                    float opacity = 1.0f;
                    float fontSize = 1.0f; // Default font size
                    TextAlign::Horizontal horizontalAlign = TextAlign::Horizontal::Left;
                    TextAlign::Vertical verticalAlign = TextAlign::Vertical::Top;
                };

                /**
                 * Arguments struct for Text constructor
                 */
                struct Args {
                    std::string text = "";
                    glm::vec2 position = glm::vec2(0.0f);
                    glm::vec2 size = glm::vec2(0.0f); 
                    Styles style = Styles({});
                    float zIndex = 0.0f;
                };

                /**
                 * Create a text shape using the Args struct.
                 *
                 * @param args A struct containing all arguments for the text.
                 */
                explicit Text(const Args &args);

                virtual ~Text() = default;

                // Getters and setters
                const std::string &getText() const { return text; }
                void setText(const std::string &t) {
                    text = t;
                    markDirty();
                }

                const Styles &getStyle() const { return style; }
                void setStyle(const Styles &s) {
                    style = s;
                    markDirty();
                }

                const glm::vec2 &getSize() const { return size; }
                void setSize(const glm::vec2 &s) {
                    size = s;
                    markDirty();
                }

                const glm::vec2 &getPosition() const { return position; }
                void setPosition(const glm::vec2 &s) {
                    position = s;
                    markDirty();
                }
                

                virtual void draw() override;

            private:
                std::string text;
                Styles style;
                glm::vec2 position;
                float zIndex;
                glm::vec2 size; 

        };

    } // namespace Shapes
} // namespace Rendering