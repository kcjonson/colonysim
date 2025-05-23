#pragma once

#include "Rendering/Layer.h"
#include "Rendering/Shapes/Rectangle.h"
#include "Rendering/Shapes/Text.h"
#include "Rendering/Styles/Shape.h"
#include "Rendering/Styles/BorderPosition.h"
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace Rendering {
    namespace Components {
        namespace Form {            /**
             * A text input component with a label and onChange callback
             *
             * The text input uses a rectangle for its background, a text
             * for its label, and allows user to input text. It provides
             * a callback function when the text changes.
             */            class Text : public Layer, public std::enable_shared_from_this<Text> {
              public:
                // Static pointer to the currently focused text input
                static std::shared_ptr<Text> focusedTextInput;
                
                // Parameter struct for text input styling
                struct StyleParams {
                    glm::vec4 color = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);          // Background color
                    float opacity = 1.0f;
                    glm::vec4 borderColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
                    float borderWidth = 1.0f;
                    BorderPosition borderPosition = BorderPosition::Outside;
                    float cornerRadius = 3.0f;
                    glm::vec4 textColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);         // Text color
                    glm::vec4 placeholderColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);   // Placeholder text color
                    glm::vec4 focusColor = glm::vec4(0.8f, 0.9f, 1.0f, 1.0f);         // Background when focused
                    glm::vec4 focusBorderColor = glm::vec4(0.4f, 0.6f, 0.9f, 1.0f);   // Border when focused
                };                // Text input style struct
                struct Styles {
                    glm::vec4 color = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
                    float opacity = 1.0f;
                    glm::vec4 borderColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
                    float borderWidth = 1.0f;
                    BorderPosition borderPosition = BorderPosition::Outside;
                    float cornerRadius = 3.0f;
                    glm::vec4 textColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
                    glm::vec4 placeholderColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                    glm::vec4 focusColor = glm::vec4(0.8f, 0.9f, 1.0f, 1.0f);
                    glm::vec4 focusBorderColor = glm::vec4(0.4f, 0.6f, 0.9f, 1.0f);
                    
                    Styles() = default;
                    
                    Styles(const StyleParams& params)
                        : color(params.color),
                        opacity(params.opacity),
                        borderColor(params.borderColor),
                        borderWidth(params.borderWidth),
                        borderPosition(params.borderPosition),
                        cornerRadius(params.cornerRadius),
                        textColor(params.textColor),
                        placeholderColor(params.placeholderColor),
                        focusColor(params.focusColor),
                        focusBorderColor(params.focusBorderColor) {
                    }
                };
                  // New struct for Text arguments
                struct Args {
                    std::string label = "";
                    std::string placeholder = "";
                    std::string value = "";
                    glm::vec2 position = glm::vec2(0.0f);
                    glm::vec2 size = glm::vec2(200.0f, 30.0f);
                    Styles style = {};
                    bool disabled = false;
                    float zIndex = 0.0f;
                    std::function<void(const std::string&)> onChange = nullptr;
                };

                /**
                 * Create a text input using the Text::Args struct.
                 *
                 * @param args A struct containing all arguments for the text input.
                 */
                explicit Text(const Args &args);

                virtual ~Text() = default;

                // Getters and setters
                
                // Value
                const std::string &getValue() const { return value; }
                void setValue(const std::string &newValue);

                // Size
                const glm::vec2 &getSize() const { return size; }
                void setSize(const glm::vec2 &newSize);

                // Style
                const Styles &getStyle() const { return style; }
                void setStyle(const Styles &s);

                // Label
                const std::string &getLabel() const { return label; }
                void setLabel(const std::string &text);

                // Placeholder
                const std::string &getPlaceholder() const { return placeholder; }
                void setPlaceholder(const std::string &text);

                // onChange callback
                const std::function<void(const std::string&)> &getOnChange() const { return onChange; }
                void setOnChange(const std::function<void(const std::string&)> &callback) { onChange = callback; }

                // Disabled
                bool isDisabled() const { return disabled; }
                void setDisabled(bool d);

                // Position
                const glm::vec2 &getPosition() const { return position; }
                void setPosition(const glm::vec2 &pos);

                // Focus state
                bool isFocused() const { return focused; }
                void setFocus(bool focus);                // Implementation of the render method (overriding Layer)
                virtual void render(bool batched = false) override;

                // Input handling methods
                void handleInput(float deltaTime = 0.0f) override;
                
                // Process keyboard input
                void handleKeyInput(int key, int scancode, int action, int mods);
                void handleCharInput(unsigned int codepoint);

              private:

                // Implementation of the draw method (no longer overriding)
                void draw();

                // Process mouse input
                void handleMouseMove(const glm::vec2 &mousePos);
                void handleMouseButton(int button, int action);

                // Check if a point is within the input's bounds
                bool containsPoint(const glm::vec2 &point) const;
                
                // Helper function to convert Text style to Rectangle style
                Rendering::Styles::Rectangle textToRectangleStyle(const Styles &textStyle, bool forFocus = false);

                // Update the visual state of the text input
                void updateVisualState();

                // Get predefined styles for different states
                static Styles getDisabledStyle();
                static Styles getFocusStyle(const Styles &baseStyle);

                // Apply the onChange callback
                void applyOnChange();

                // Mark the text input as dirty when properties change
                void markDirty() { dirty = true; }

                // Insert text at cursor position
                void insertTextAtCursor(const std::string &text);
                
                // Delete text based on cursor position
                void deleteTextBeforeCursor();
                void deleteTextAfterCursor();

                // Move cursor
                void moveCursorLeft();
                void moveCursorRight();
                void moveCursorToStart();
                void moveCursorToEnd();

                glm::vec2 position;
                std::string label;
                std::string placeholder;
                std::string value;
                glm::vec2 size;
                Styles style;
                std::function<void(const std::string&)> onChange;
                bool disabled = false;
                bool dirty = true; // Dirty flag

                // Internal components
                std::shared_ptr<Shapes::Rectangle> background;
                std::shared_ptr<Shapes::Text> labelText;
                std::shared_ptr<Shapes::Text> inputText;
                std::shared_ptr<Shapes::Rectangle> cursor;

                // Input state tracking
                bool focused = false;
                bool mouseOver = false;
                bool mouseDown = false;
                size_t cursorPosition = 0;
                float cursorBlinkTimer = 0.0f;
                bool cursorVisible = true;
                float horizontalOffset = 0.0f; // Horizontal offset for scrolling text
                glm::vec2 inputTextBasePosition; // Base position for input text to apply scrolling offset
            };

        } // namespace Form
    } // namespace Components
} // namespace Rendering
