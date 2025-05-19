#pragma once

#include "./ButtonStyle.h"
#include "Rendering/Layer.h"
#include "Rendering/Shapes/Rectangle.h"
#include "Rendering/Shapes/Text.h"
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace Rendering {
	namespace Components {

		// New struct for Button arguments
		struct ButtonArgs {
			std::string label = "";
			glm::vec2 position = glm::vec2(0.0f);
			glm::vec2 size = glm::vec2(100.0f, 30.0f);
			Styles::Button style = Styles::Button({});
			Styles::Button hoverStyle = Styles::Button({});
			Styles::Button pressedStyle = Styles::Button({});
			float zIndex = 0.0f;
			std::function<void()> onClick = nullptr;
		};

		/**
		 * A button component with a label and onClick callback
		 *
		 * The button uses a rectangle for its background and a text
		 * for its label. It also provides a callback function for click events.
		 */
		class Button : public Layer {
		  public:
			// Button states for visual feedback
			enum class State { Normal, Hover, Pressed };

			/**
			 * Create a button using the ButtonArgs struct.
			 *
			 * @param args A struct containing all arguments for the button.
			 */
			explicit Button(const ButtonArgs &args);

			virtual ~Button() = default;

			// Getters and setters
			const glm::vec2 &getSize() const { return size; }
			void setSize(const glm::vec2 &newSize);

			// Style
			const Styles::Button &getStyle() const { return style; }
			void setStyle(const Styles::Button &s);

			// Label
			const std::string &getLabel() const { return label; }
			void setLabel(const std::string &text);

			// onClick callback
			const std::function<void()> &getOnClick() const { return onClick; }
			void setOnClick(const std::function<void()> &callback) { onClick = callback; }

			// Position
			const glm::vec2 &getPosition() const { return position; }
			void setPosition(const glm::vec2 &pos);

			// Implementation of the render method (overriding Layer)
			virtual void render(bool batched = false) override;

			// Trigger the click event directly
			void click(); // Input handling methods
			void handleInput(float deltaTime = 0.0f);


		  private:

			// Implementation of the draw method (no longer overriding)
			void draw();

			// Process mouse input
			void handleMouseMove(const glm::vec2 &mousePos);
			void handleMouseButton(int button, int action);

			// Check if a point is within the button's bounds
			bool containsPoint(const glm::vec2 &point) const;

			
			// State accessors
			State getState() const { return state; }
			bool isHovered() const { return state == State::Hover || state == State::Pressed; }
			bool isPressed() const { return state == State::Pressed; }

			// Helper function to convert Button style to Rectangle style
			Styles::Rectangle buttonToRectangleStyle(const Styles::Button &buttonStyle);

			// Update the visual state of the button
			void updateVisualState();

			// Mark the button as dirty when properties change (similar to Shape::markDirty)
			void markDirty() { dirty = true; }

			glm::vec2 position;
			std::string label;
			glm::vec2 size;
			Styles::Button style;
			Styles::Button hoverStyle;
			Styles::Button pressedStyle;
			std::function<void()> onClick;
			bool dirty = true; // Dirty flag (moved from Shape)

			// Internal components
			std::shared_ptr<Shapes::Rectangle> background;
			std::shared_ptr<Shapes::Text> labelText;

			// Input state tracking
			State state = State::Normal;
			bool mouseOver = false;
			bool mouseDown = false;
		};

	} // namespace Components
} // namespace Rendering