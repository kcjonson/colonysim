#include "Rendering/Components/Form/Text.h"
#include <GLFW/glfw3.h> // For input constants
#include <glad/glad.h> // For OpenGL scissor
#include <algorithm>    // For std::min/max

namespace Rendering {
    namespace Components {
        namespace Form {
            
            // Initialize the static pointer
            std::shared_ptr<Text> Text::focusedTextInput;

            Text::Text(const Args &args)
                : Layer(args.zIndex, ProjectionType::ScreenSpace, nullptr, glfwGetCurrentContext()),
                  position(args.position),
                  label(args.label),
                  placeholder(args.placeholder),
                  value(args.value),
                  size(args.size),
                  style(args.style),
                  onChange(args.onChange),
                  dirty(true) {

                this->disabled = args.disabled;

                // The Text's Layer zIndex is args.zIndex.
                // Make background at args.zIndex, labelText and inputText slightly in front,
                // and cursor at the very front.
                float backgroundZ = args.zIndex;
                float labelTextZ = args.zIndex + 1.0f;
                float inputTextZ = args.zIndex + 1.0f;
                float cursorZ = args.zIndex + 2.0f;

                // Create the background rectangle
                background = std::make_shared<Shapes::Rectangle>(
                    Shapes::Rectangle::Args{
                        .position = this->position,
                        .size = this->size,
                        .style = textToRectangleStyle(this->style),
                        .zIndex = backgroundZ
                    }
                );

                // Create label text if provided
                if (!this->label.empty()) {
                    // Position label text above the input field
                    glm::vec2 labelPosition = this->position;
                    labelPosition.y -= 20.0f; // Adjust based on your design

                    labelText = std::make_shared<Shapes::Text>(Shapes::Text::Args{
                        .text = this->label,
                        .position = labelPosition,
                        .style = Shapes::Text::Styles(
                            {.color = this->style.textColor,
                             .fontSize = 0.9f, // Slightly smaller than input text
                             .horizontalAlign = TextAlign::Horizontal::Left,
                             .verticalAlign = TextAlign::Vertical::Bottom}
                        ),
                        .zIndex = labelTextZ
                    });
                }

                // Calculate input text position (a bit inset from the edges)
                glm::vec2 inputPosition = this->position;
                inputPosition.x += 5.0f; // Left padding
                inputPosition.y += 5.0f; // Top padding

                // Create input text
                inputText = std::make_shared<Shapes::Text>(Shapes::Text::Args{
                    .text = this->value.empty() ? this->placeholder : this->value,
                    .position = inputPosition,
                    .size = glm::vec2(this->size.x - 10.0f, this->size.y - 10.0f), // Adjust for padding
                    .style = Shapes::Text::Styles(
                        {.color = this->value.empty() ? this->style.placeholderColor : this->style.textColor,
                         .fontSize = 1.0f,
                         .horizontalAlign = TextAlign::Horizontal::Left,
                         .verticalAlign = TextAlign::Vertical::Middle}
                    ),
                    .zIndex = inputTextZ
                });
                // Store base position for scrolling
                inputTextBasePosition = inputPosition;
                
                // Create cursor for text editing
                cursor = std::make_shared<Shapes::Rectangle>(
                    Shapes::Rectangle::Args{
                        .position = inputPosition, // Will be updated in updateVisualState
                        .size = glm::vec2(1.0f, this->size.y - 14.0f), // Thin vertical line, slightly shorter than input height
                        .style = Rendering::Styles::Rectangle({
                            .color = this->style.textColor,
                            .opacity = 1.0f
                        }),
                        .zIndex = cursorZ
                    }
                );
                cursor->setVisible(false); // Initially invisible until focused
            }

            void Text::setValue(const std::string &newValue) {
                if (value != newValue) {
                    value = newValue;
                    if (inputText) {
                        inputText->setText(value.empty() ? placeholder : value);
                        // Update text color based on whether there's value or placeholder
                        auto textStyle = inputText->getStyle();
                        textStyle.color = value.empty() ? style.placeholderColor : style.textColor;
                        inputText->setStyle(textStyle);
                    }
                    markDirty();
                    applyOnChange();
                }
            }

            void Text::setSize(const glm::vec2 &newSize) {
                size = newSize;
                if (background) {
                    background->setSize(newSize);
                    
                    // Update input text size (with padding)
                    if (inputText) {
                        inputText->setSize(glm::vec2(newSize.x - 10.0f, newSize.y - 10.0f));
                    }
                    
                    // Update cursor size
                    if (cursor) {
                        cursor->setSize(glm::vec2(1.0f, newSize.y - 14.0f));
                    }
                }
                markDirty();
            }

            void Text::setStyle(const Styles &s) {
                style = s;
                if (background) {
                    background->setStyle(textToRectangleStyle(s));
                }
                
                if (inputText) {
                    auto textStyle = inputText->getStyle();
                    textStyle.color = value.empty() ? s.placeholderColor : s.textColor;
                    inputText->setStyle(textStyle);
                }
                
                if (labelText) {
                    auto labelStyle = labelText->getStyle();
                    labelStyle.color = s.textColor;
                    labelText->setStyle(labelStyle);
                }
                
                if (cursor) {
                    auto cursorStyle = cursor->getStyle();
                    cursorStyle.color = s.textColor;
                    cursor->setStyle(cursorStyle);
                }
                
                markDirty();
            }

            void Text::setLabel(const std::string &text) {
                label = text;
                if (labelText) {
                    labelText->setText(text);
                } else if (!text.empty()) {
                    // Create label text if it doesn't exist but was now provided
                    glm::vec2 labelPosition = this->position;
                    labelPosition.y -= 20.0f;
                    
                    labelText = std::make_shared<Shapes::Text>(Shapes::Text::Args{
                        .text = text,
                        .position = labelPosition,
                        .style = Shapes::Text::Styles(
                            {.color = this->style.textColor,
                             .fontSize = 0.9f,
                             .horizontalAlign = TextAlign::Horizontal::Left,
                             .verticalAlign = TextAlign::Vertical::Bottom}
                        ),
                        .zIndex = getZIndex() + 1.0f
                    });
                }
                markDirty();
            }

            void Text::setPlaceholder(const std::string &text) {
                placeholder = text;
                if (value.empty() && inputText) {
                    inputText->setText(placeholder);
                    auto textStyle = inputText->getStyle();
                    textStyle.color = style.placeholderColor;
                    inputText->setStyle(textStyle);
                }
                markDirty();
            }

            void Text::setDisabled(bool d) {
                if (disabled != d) {
                    disabled = d;
                    // Reset state when changing disabled state
                    if (disabled) {
                        setFocus(false);
                    }
                    updateVisualState();
                    markDirty();
                }
            }

            void Text::setPosition(const glm::vec2 &pos) {
                glm::vec2 offset = pos - position;
                position = pos;
                
                if (background) {
                    background->setPosition(pos);
                }
                
                if (labelText) {
                    // Keep the relative position of the label
                    labelText->setPosition(labelText->getPosition() + offset);
                }
                
                if (inputText) {
                    // Keep the relative position of the input text
                    inputText->setPosition(inputText->getPosition() + offset);
                    // Update base position for scrolling
                    inputTextBasePosition += offset;
                }
                
                if (cursor && focused) {
                    // Keep the relative position of the cursor
                    cursor->setPosition(cursor->getPosition() + offset);
                }
                
                markDirty();
            }            void Text::setFocus(bool focus) {
                if (focused != focus && !disabled) {
                    focused = focus;
                    if (focused) {
                        // Position cursor at the end of text when focusing
                        cursorPosition = value.length();
                        cursorBlinkTimer = 0.0f;
                        cursorVisible = true;
                        
                        // Set this as the currently focused text input
                        focusedTextInput = shared_from_this();
                    } else {
                        // Hide cursor when losing focus
                        cursor->setVisible(false);
                        
                        // If this was the focused input, clear the static pointer
                        if (focusedTextInput.get() == this) {
                            focusedTextInput = nullptr;
                        }
                    }
                    updateVisualState();
                    markDirty();
                }
            }

            void Text::render(bool batched) {
                if (!visible)
                    return;
                draw();
            }

            void Text::draw() {
                // Draw in order: background, label (if any), input text, cursor (if focused)
                background->draw();
                
                if (labelText) {
                    labelText->draw();
                }
                
                // Apply scissor mask for input field region
                GLFWwindow* window = glfwGetCurrentContext();
                // Get window and framebuffer sizes for DPI scaling
                int winW, winH, fbW, fbH;
                glfwGetWindowSize(window, &winW, &winH);
                glfwGetFramebufferSize(window, &fbW, &fbH);
                float scaleX = (float)fbW / (float)winW;
                float scaleY = (float)fbH / (float)winH;
                // Compute scissor rectangle in framebuffer coords
                // Include viewport origin
                GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
                int scX = vp[0] + static_cast<int>(position.x * scaleX);
                int scY = vp[1] + static_cast<int>(fbH - (position.y + size.y) * scaleY);
                int scW = static_cast<int>(size.x * scaleX);
                int scH = static_cast<int>(size.y * scaleY);
                glEnable(GL_SCISSOR_TEST);
                glScissor(scX, scY, scW, scH);
                // Draw text and cursor within mask
                inputText->draw();
                if (focused && cursorVisible) {
                    cursor->draw();
                }
                glDisable(GL_SCISSOR_TEST);
            }

            bool Text::containsPoint(const glm::vec2 &point) const {
                return point.x >= position.x && point.x <= position.x + size.x && 
                       point.y >= position.y && point.y <= position.y + size.y;
            }

            Rendering::Styles::Rectangle Text::textToRectangleStyle(const Styles &textStyle, bool forFocus) {
                Rendering::Styles::RectangleStyleParams rectParams;
                
                if (forFocus) {
                    rectParams.color = textStyle.focusColor;
                    rectParams.borderColor = textStyle.focusBorderColor;
                } else {
                    rectParams.color = textStyle.color;
                    rectParams.borderColor = textStyle.borderColor;
                }
                
                rectParams.opacity = textStyle.opacity;
                rectParams.borderWidth = textStyle.borderWidth;
                rectParams.borderPosition = textStyle.borderPosition;
                rectParams.cornerRadius = textStyle.cornerRadius;
                
                return Rendering::Styles::Rectangle(rectParams);
            }

            Text::Styles Text::getDisabledStyle() {
                StyleParams params;
                
                // Grey background for disabled state
                params.color = glm::vec4(0.85f, 0.85f, 0.85f, 1.0f);
                params.borderColor = glm::vec4(0.75f, 0.75f, 0.75f, 1.0f);
                params.textColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                params.placeholderColor = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
                
                return Styles(params);
            }

            Text::Styles Text::getFocusStyle(const Styles &baseStyle) {
                // Create a style for focus state
                Text::Styles focusStyle = baseStyle;
                
                // Use focus-specific colors if available, otherwise derive from base style
                if (baseStyle.focusColor == glm::vec4(0.8f, 0.9f, 1.0f, 1.0f)) {
                    focusStyle.color = glm::vec4(
                        std::min(baseStyle.color.r + 0.05f, 1.0f),
                        std::min(baseStyle.color.g + 0.05f, 1.0f),
                        std::min(baseStyle.color.b + 0.1f, 1.0f),
                        baseStyle.color.a
                    );
                } else {
                    focusStyle.color = baseStyle.focusColor;
                }
                
                if (baseStyle.focusBorderColor == glm::vec4(0.4f, 0.6f, 0.9f, 1.0f)) {
                    focusStyle.borderColor = glm::vec4(0.4f, 0.6f, 0.9f, 1.0f); // Blue focus border
                } else {
                    focusStyle.borderColor = baseStyle.focusBorderColor;
                }
                
                // Make border slightly thicker for focus state
                focusStyle.borderWidth = baseStyle.borderWidth * 1.5f;
                
                return focusStyle;
            }

            void Text::applyOnChange() {
                if (onChange) {
                    onChange(value);
                }
            }

            void Text::handleInput(float deltaTime) {
                // Skip input handling if disabled
                if (disabled) return;
                
                // Need a window reference to get input state
                GLFWwindow *window = glfwGetCurrentContext();
                if (!window) return;
                
                // Get current mouse position
                double x, y;
                glfwGetCursorPos(window, &x, &y);
                glm::vec2 mousePos(static_cast<float>(x), static_cast<float>(y));
                
                // Handle mouse movement
                handleMouseMove(mousePos);
                
                // Handle mouse button state
                int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
                handleMouseButton(GLFW_MOUSE_BUTTON_LEFT, state);
                
                // Handle cursor blinking if focused
                if (focused) {
                    cursorBlinkTimer += deltaTime;
                    if (cursorBlinkTimer > 0.5f) { // Blink every half second
                        cursorBlinkTimer = 0.0f;
                        cursorVisible = !cursorVisible;
                        cursor->setVisible(cursorVisible);
                    }
                }
            }

            void Text::handleMouseMove(const glm::vec2 &mousePos) {
                bool wasOver = mouseOver;
                mouseOver = containsPoint(mousePos);
                
                // Update label hover state too if it exists
                if (labelText) {
                    // Simple check if mouse is over the label
                    glm::vec2 labelPos = labelText->getPosition();
                    glm::vec2 labelSize = labelText->getSize();
                    bool overLabel = mousePos.x >= labelPos.x && mousePos.x <= labelPos.x + labelSize.x &&
                                     mousePos.y >= labelPos.y && mousePos.y <= labelPos.y + labelSize.y;
                    
                    // Consider mouse over if it's over either the input or the label
                    mouseOver = mouseOver || overLabel;
                }
                
                if (mouseOver != wasOver) {
                    updateVisualState();
                }
            }

            void Text::handleMouseButton(int button, int action) {
                // Only handle left mouse button
                if (button != GLFW_MOUSE_BUTTON_LEFT) return;
                
                if (!disabled) {
                    bool wasDown = mouseDown;
                    mouseDown = (action == GLFW_PRESS);
                    
                    // If mouse is pressed over the input or its label, focus the input
                    if (!wasDown && mouseDown && mouseOver) {
                        setFocus(true);
                        
                        // TODO: Position cursor based on click position relative to text
                        // This requires more advanced text measurement
                        // For now, just move cursor to end of text
                        cursorPosition = value.length();
                        updateVisualState();
                    } else if (!mouseOver && mouseDown) {
                        // Clicked outside the input - lose focus
                        setFocus(false);
                    }
                }
            }

            void Text::handleKeyInput(int key, int scancode, int action, int mods) {
                if (!focused || disabled) return;
                
                if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                    switch (key) {
                        case GLFW_KEY_BACKSPACE:
                            deleteTextBeforeCursor();
                            break;
                        case GLFW_KEY_DELETE:
                            deleteTextAfterCursor();
                            break;
                        case GLFW_KEY_LEFT:
                            moveCursorLeft();
                            break;
                        case GLFW_KEY_RIGHT:
                            moveCursorRight();
                            break;
                        case GLFW_KEY_HOME:
                            moveCursorToStart();
                            break;
                        case GLFW_KEY_END:
                            moveCursorToEnd();
                            break;
                        case GLFW_KEY_ENTER:
                        case GLFW_KEY_ESCAPE:
                            setFocus(false);
                            break;
                    }
                    
                    // Reset cursor blink when key is pressed
                    cursorBlinkTimer = 0.0f;
                    cursorVisible = true;
                    cursor->setVisible(true);
                    
                    updateVisualState();
                }
            }

            void Text::handleCharInput(unsigned int codepoint) {
                if (!focused || disabled) return;
                
                // Convert codepoint to UTF-8 string
                // This is a simplified approach for ASCII characters
                // A proper implementation would handle all UTF-8 encoding
                if (codepoint < 128) {
                    char c = static_cast<char>(codepoint);
                    std::string charStr(1, c);
                    insertTextAtCursor(charStr);
                    
                    // Reset cursor blink
                    cursorBlinkTimer = 0.0f;
                    cursorVisible = true;
                    cursor->setVisible(true);
                    
                    updateVisualState();
                }
            }

            void Text::insertTextAtCursor(const std::string &text) {
                if (cursorPosition > value.length()) {
                    cursorPosition = value.length();
                }
                
                std::string newValue = value;
                newValue.insert(cursorPosition, text);
                cursorPosition += text.length();
                
                setValue(newValue);
            }

            void Text::deleteTextBeforeCursor() {
                if (cursorPosition > 0 && !value.empty()) {
                    std::string newValue = value;
                    newValue.erase(cursorPosition - 1, 1);
                    cursorPosition--;
                    
                    setValue(newValue);
                }
            }

            void Text::deleteTextAfterCursor() {
                if (cursorPosition < value.length() && !value.empty()) {
                    std::string newValue = value;
                    newValue.erase(cursorPosition, 1);
                    
                    setValue(newValue);
                }
            }

            void Text::moveCursorLeft() {
                if (cursorPosition > 0) {
                    cursorPosition--;
                    updateVisualState();
                }
            }

            void Text::moveCursorRight() {
                if (cursorPosition < value.length()) {
                    cursorPosition++;
                    updateVisualState();
                }
            }

            void Text::moveCursorToStart() {
                cursorPosition = 0;
                updateVisualState();
            }

            void Text::moveCursorToEnd() {
                cursorPosition = value.length();
                updateVisualState();
            }

            void Text::updateVisualState() {
                // If input is disabled, apply disabled style
                if (disabled) {
                    Styles disabledStyle = getDisabledStyle();
                    background->setStyle(textToRectangleStyle(disabledStyle));
                    
                    // Update text colors for disabled state
                    if (inputText) {
                        auto textStyle = inputText->getStyle();
                        textStyle.color = disabledStyle.textColor;
                        inputText->setStyle(textStyle);
                    }
                    
                    if (labelText) {
                        auto labelStyle = labelText->getStyle();
                        labelStyle.color = disabledStyle.textColor;
                        labelText->setStyle(labelStyle);
                    }
                    
                    return;
                }
                
                // Determine visual state based on focus
                if (focused) {
                    // Apply focus style to background
                    background->setStyle(textToRectangleStyle(style, true));
                    
                    // Show and position cursor
                    if (cursor) {
                        cursor->setVisible(cursorVisible);
                        
                        // Adjust horizontal offset to ensure cursor is visible
                        float beforeWidth = inputText->measureTextWidth(value.substr(0, cursorPosition));
                        float availWidth = size.x - 10.0f;
                        if (beforeWidth + horizontalOffset < 0.0f) {
                            horizontalOffset = -beforeWidth;
                        } else if (beforeWidth + horizontalOffset > availWidth) {
                            horizontalOffset = availWidth - beforeWidth;
                        }
                        // Optionally clamp offset to content bounds
                        float fullTextWidth = inputText->measureTextWidth(value);
                        float minOffset = std::min(0.0f, availWidth - fullTextWidth);
                        horizontalOffset = std::max(minOffset, std::min(horizontalOffset, 0.0f));
                        // Compute cursor position based on updated offset and base position
                        glm::vec2 cursorPos = inputTextBasePosition + glm::vec2(horizontalOffset + beforeWidth, 0.0f);
                        
                        // Vertical centering
                        float textHeight = inputText->getStyle().fontSize * 16.0f; // Approximate line height
                        cursorPos.y = inputTextBasePosition.y + (inputText->getSize().y - textHeight) / 2.0f;
                        cursor->setPosition(cursorPos);
                    }
                } else {
                    // Normal style
                    background->setStyle(textToRectangleStyle(style, false));
                    
                    // Hide cursor when not focused
                    if (cursor) {
                        cursor->setVisible(false);
                    }
                }
                
                // Update text colors and content
                if (inputText) {
                    auto textStyle = inputText->getStyle();
                    
                    // If there's no value, show placeholder text
                    if (value.empty()) {
                        inputText->setText(placeholder);
                        textStyle.color = style.placeholderColor;
                    } else {
                        inputText->setText(value);
                        textStyle.color = style.textColor;
                    }
                    
                    inputText->setStyle(textStyle);
                    
                    // Apply horizontal offset for scrolling based on base position
                    glm::vec2 newPos = inputTextBasePosition + glm::vec2(horizontalOffset, 0.0f);
                    inputText->setPosition(newPos);
                }
                
                markDirty();
            }
            
        } // namespace Form
    } // namespace Components
} // namespace Rendering
