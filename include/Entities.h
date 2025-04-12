#pragma once

#include <vector>
#include <memory>
#include "Entity.h"
#include "VectorGraphics.h"
#include "Rendering/Layer.h"
#include "Camera.h"
#include "Renderer.h"
#include <GLFW/glfw3.h>

class Entities {
public:
    Entities();
    ~Entities();

    void update(float deltaTime);
    void render(VectorGraphics& graphics);

    // Entity management
    size_t createEntity(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void removeEntity(size_t index);
    Entity* getEntity(size_t index);
    const Entity* getEntity(size_t index) const;
    size_t getEntityCount() const { return entities.size(); }

    // Entity control
    void moveEntity(size_t index, const glm::vec2& target);
    void setEntityState(size_t index, EntityState state);
    
    // Set camera, window, and renderer
    void setCamera(Camera* cam);
    void setWindow(GLFWwindow* win);
    void setRenderer(Renderer* renderer);

private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::shared_ptr<Rendering::Layer> entityLayer;

    // Update methods
    void updateMovement(float deltaTime);
    void updateWork(float deltaTime);
}; 