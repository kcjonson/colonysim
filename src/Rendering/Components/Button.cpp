#include "Rendering/Components/Button.h"
#include "Rendering/Styles/Shape.h"
#include <GLFW/glfw3.h> // For input constants
#include <algorithm>	// For std::min/max

namespace Rendering {
	namespace Components {

		Button::Button(const Args &args)
			: Layer(args.zIndex),
			  position(args.position),
			  label(args.label),
			  size(args.size),
			  type(args.type),
			  style(args.style),
			  hoverStyle(args.hoverStyle),
			  pressedStyle(args.pressedStyle),
			  onClick(args.onClick),
			  dirty(true) {


			this->disabled = args.disabled;
			// Apply predefined styles based on type if not custom
			if (type != Type::Custom) {
				applyTypeStyles();
			} else {
				// If custom type but using default styles, apply Primary style
				if (this->style.color == glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)) {
					this->style = getDefaultStyleForType(Type::Primary);
				}

				// If hover style was default-initialized, create a slightly lighter version of the normal style
				if (this->hoverStyle.color == glm::vec4(1.0f)) {
					auto derivedHoverStyle = this->style; // Start with the normal style
					derivedHoverStyle.color = glm::vec4(
						std::min(this->style.color.r + 0.1f, 1.0f),
						std::min(this->style.color.g + 0.1f, 1.0f),
						std::min(this->style.color.b + 0.1f, 1.0f),
						this->style.color.a
					);
					this->hoverStyle = derivedHoverStyle;
				}

				// If pressed style was default-initialized, create a slightly darker version of the normal style
				if (this->pressedStyle.color == glm::vec4(1.0f)) {
					auto derivedPressedStyle = this->style; // Start with the normal style
					derivedPressedStyle.color = glm::vec4(
						std::max(this->style.color.r - 0.1f, 0.0f),
						std::max(this->style.color.g - 0.1f, 0.0f),
						std::max(this->style.color.b - 0.1f, 0.0f),
						this->style.color.a
					);
					this->pressedStyle = derivedPressedStyle;
				}
			}

			// The Button's Layer zIndex is args.zIndex.
			// Make background be at args.zIndex and labelText slightly in
			// front. This ensures non-negative z-indices for parts if
			// args.zIndex is 0.
			float backgroundZ = args.zIndex;
			float labelTextZ = args.zIndex + 1.0f; // Small positive offset for text

			background = std::make_shared<Shapes::Rectangle>(
				Shapes::Rectangle::Args{
					.position = this->position,
					.size = this->size,
					.style = buttonToRectangleStyle(this->style)
				}
			);

			labelText = std::make_shared<Shapes::Text>(Shapes::Text::Args{
				.text = this->label,
				.position = this->position,
				.size = this->size,
				.style = Shapes::Text::Styles(
					{.color = glm::vec4(1.0f),
					 .fontSize = 1.0f,
					 .horizontalAlign = TextAlign::Horizontal::Center,
					 .verticalAlign = TextAlign::Vertical::Middle}
				)
			});
		}

		void Button::applyTypeStyles() {
			style = getDefaultStyleForType(type);
			hoverStyle = getHoverStyleForType(type);
			pressedStyle = getPressedStyleForType(type);
			
			// Update rectangle style if background already exists
			if (background) {
				background->setStyle(buttonToRectangleStyle(style));
			}
			
			markDirty();
		}

		void Button::setType(Type newType) {
			if (type != newType) {
				type = newType;
				applyTypeStyles();
				updateVisualState(); // Apply the correct style based on current state
			}
		}

		Button::Styles Button::getDefaultStyleForType(Type type) {
			Rendering::Styles::ButtonStyleParams params;
			
			switch (type) {
				case Type::Primary:
					// Blue primary button
					params.color = glm::vec4(0.2f, 0.4f, 0.8f, 1.0f);
					params.borderColor = glm::vec4(0.1f, 0.2f, 0.5f, 1.0f);
					params.borderWidth = 1.0f;
					params.cornerRadius = 5.0f;
					break;
				case Type::Secondary:
					// Light blue secondary button
					params.color = glm::vec4(0.5f, 0.7f, 0.9f, 1.0f);
					params.borderColor = glm::vec4(0.4f, 0.6f, 0.8f, 1.0f);
					params.borderWidth = 1.0f;
					params.cornerRadius = 5.0f;
					break;
				case Type::Custom:
				default:
					// Default style (shouldn't be used with Custom type)
					params.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
					params.borderWidth = 0.0f;
					params.cornerRadius = 5.0f;
					break;
			}
			
			return Button::Styles(params);
		}
		
		Button::Styles Button::getHoverStyleForType(Type type) {
			Button::Styles baseStyle = getDefaultStyleForType(type);
			
			// Create a slightly lighter version for hover state
			baseStyle.color = glm::vec4(
				std::min(baseStyle.color.r + 0.1f, 1.0f),
				std::min(baseStyle.color.g + 0.1f, 1.0f),
				std::min(baseStyle.color.b + 0.1f, 1.0f),
				baseStyle.color.a
			);
			
			return baseStyle;
		}

		Button::Styles Button::getPressedStyleForType(Type type) {
			Button::Styles baseStyle = getDefaultStyleForType(type);
			
			// Create a slightly darker version for pressed state
			baseStyle.color = glm::vec4(
				std::max(baseStyle.color.r - 0.1f, 0.0f),
				std::max(baseStyle.color.g - 0.1f, 0.0f),
				std::max(baseStyle.color.b - 0.1f, 0.0f),
				baseStyle.color.a
			);
			
			return baseStyle;
		}

		Button::Styles Button::getDisabledStyle() {
			Rendering::Styles::ButtonStyleParams params;
			
			// Grey background for disabled state
			params.color = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
			params.borderColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
			params.borderWidth = 1.0f;
			params.cornerRadius = 5.0f;
			
			return Button::Styles(params);
		}

		void Button::setSize(const glm::vec2 &newSize) {
			size = newSize;
			if (background) {
				background->setSize(newSize);
				labelText->setSize(newSize);
			}
			markDirty();
		}

		void Button::setPosition(const glm::vec2 &pos) {
			position = pos;
			if (background) {
				background->setPosition(pos);
				labelText->setPosition(pos);
			}
			markDirty();
		}

		void Button::setStyle(const Styles &s) {
			style = s;
			// When manually setting style, change to custom type
			type = Type::Custom;
			if (background) {
				background->setStyle(buttonToRectangleStyle(s));
			}
			markDirty();
		}

		void Button::setDisabled(bool d) {
			if (disabled != d) {
				disabled = d;
				// Reset mouse state when changing disabled state
				if (disabled) {
					mouseDown = false;
					// Force state to normal when disabling
					state = State::Normal;
				} else {
					// Force state update when enabling by setting to an invalid state first
					State currentState = state;
					state = static_cast<State>(-1); // Force the state to be different
					updateVisualState();
					// The updateVisualState call will set the correct state
				}
				updateVisualState();
				markDirty();
			}
		}

		void Button::setLabel(const std::string &text) {
			label = text;
			if (labelText) {
				labelText->setText(text);
			}
			markDirty();
		}

		void Button::render(bool batched) {
			if (!visible)
				return;
			draw();
		}

		void Button::draw() {
			// Draw the background rectangle
			background->draw();
			labelText->draw();
		}
		void Button::click() {
			// Execute the onClick callback if it exists and the button is not disabled
			if (onClick && !disabled) {
				onClick();
			}
		}

		bool Button::containsPoint(const glm::vec2 &point) const {
			return point.x >= position.x && point.x <= position.x + size.x && point.y >= position.y && point.y <= position.y + size.y;
		}
		void Button::handleMouseMove(const glm::vec2 &mousePos) {
			// Check if the mouse is over the button
			bool wasOver = mouseOver;
			mouseOver = containsPoint(mousePos);

			// Update state if necessary
			if (mouseOver != wasOver) {
				updateVisualState();
			}
		}
		void Button::handleMouseButton(int button, int action) {
			// Only handle left mouse button
			if (button != 0) { // 0 is GLFW_MOUSE_BUTTON_LEFT
				return;
			}

			// If the button is disabled, don't change the mouseDown state
			if (!disabled) {
				bool wasDown = mouseDown;
				mouseDown = (action == 1); // 1 is GLFW_PRESS

				// If mouse is released over the button and was previously pressed,
				// trigger click
				if (wasDown && !mouseDown && mouseOver) {
					click();
				}
			}

			// Update visual state
			updateVisualState();
		}
		// Helper function to convert Button style to Rectangle style
		Rendering::Styles::Rectangle Button::buttonToRectangleStyle(const Styles &buttonStyle) {
			Rendering::Styles::RectangleStyleParams rectParams;
			rectParams.color = buttonStyle.color;
			rectParams.opacity = buttonStyle.opacity;
			rectParams.borderColor = buttonStyle.borderColor;
			rectParams.borderWidth = buttonStyle.borderWidth;
			rectParams.borderPosition = buttonStyle.borderPosition;
			rectParams.cornerRadius = buttonStyle.cornerRadius;
			return Rendering::Styles::Rectangle(rectParams);
		}
		void Button::updateVisualState() {
			// If button is disabled, apply disabled style and return
			if (disabled) {
				background->setStyle(buttonToRectangleStyle(getDisabledStyle()));
				
				// Update text color for disabled state
				auto textStyle = labelText->getStyle();
				textStyle.color = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f); // Dark grey text
				labelText->setStyle(textStyle);
				return;
			}

			// Determine the new state
			State newState;

			if (mouseDown && mouseOver) {
				newState = State::Pressed;
			} else if (mouseOver) {
				newState = State::Hover;
			} else {
				newState = State::Normal;
			}

			// Only update if state changed
			if (state != newState) {
				state = newState;

				// Update the background style based on state
				switch (state) {
					case State::Normal:
						background->setStyle(buttonToRectangleStyle(style));
						break;
					case State::Hover:
						background->setStyle(buttonToRectangleStyle(hoverStyle));
						break;
					case State::Pressed:
						background->setStyle(buttonToRectangleStyle(pressedStyle));
						break;
				}
				
				// Reset text color for normal states
				auto textStyle = labelText->getStyle();
				textStyle.color = glm::vec4(1.0f); // White text
				labelText->setStyle(textStyle);
			}
		}

		void Button::handleInput(float deltaTime) {
			// This method should be called by the parent layer/system each
			// frame It processes the input from GLFW for this button

			// Need a window reference to get input state
			GLFWwindow *window = glfwGetCurrentContext();
			if (!window) {
				return;
			}

			// Get current mouse position
			double x, y;
			glfwGetCursorPos(window, &x, &y);
			glm::vec2 mousePos(static_cast<float>(x), static_cast<float>(y));

			// Check if we need to transform coordinates (e.g., from screen to
			// world space) This would depend on your rendering system

			// Handle mouse movement
			handleMouseMove(mousePos);

			// Handle mouse button state
			int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
			handleMouseButton(GLFW_MOUSE_BUTTON_LEFT, state);
		}

	} // namespace Components
} // namespace Rendering