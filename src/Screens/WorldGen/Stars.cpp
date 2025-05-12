#include "Stars.h"
#include <random>
#include "../../Camera.h"
#include <GLFW/glfw3.h>
#include "../../Rendering/Layer.h"
#include "../../Rendering/Shapes/Rectangle.h"

namespace WorldGen {

Stars::Stars(Camera* camera, GLFWwindow* window)
    : m_camera(camera), m_window(window) {
    
    // Initialize star layer at z-index -100.0f for background
    m_starLayer = std::make_shared<Rendering::Layer>(-100.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
}

void Stars::generate(int width, int height) {
    // Clear the star layer
    m_starLayer->clearItems();
    
    // Create star background
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> disX(0.0f, static_cast<float>(width));
    std::uniform_real_distribution<float> disY(0.0f, static_cast<float>(height));
    std::uniform_real_distribution<float> disSize(1.0f, 3.0f);
    std::uniform_real_distribution<float> disAlpha(0.5f, 1.0f);
    
    for (int i = 0; i < 200; ++i) {
        float x = disX(gen);
        float y = disY(gen);
        float size = disSize(gen);
        float alpha = disAlpha(gen);
        
        auto star = std::make_shared<Rendering::Shapes::Rectangle>(
            glm::vec2(x, y),
            glm::vec2(size, size),
            Rendering::Styles::Rectangle({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, alpha)
            }),
            -100.0f  // Z-index matching starLayer
        );
        m_starLayer->addItem(star);
    }
}

void Stars::render() {
    // Render the star layer
    m_starLayer->render();
}

} // namespace WorldGen
