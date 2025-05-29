#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "../../Rendering/Layer.h"
#include "../../Rendering/Shapes/Rectangle.h"
#include "../../Camera.h"
#include <GLFW/glfw3.h>

// Forward declarations
class Camera;
struct GLFWwindow;

namespace WorldGen {

class Stars {
public:
    Stars(Camera* camera, GLFWwindow* window);
    ~Stars() = default;

    // Generate and render star background
    void generate(int width, int height);
    void render();

    // Get layer for external operations
    std::shared_ptr<Rendering::Layer> getLayer() const { return starLayer; }

private:
    std::shared_ptr<Rendering::Layer> starLayer;
    Camera* camera;
    GLFWwindow* window;
};

} // namespace WorldGen
