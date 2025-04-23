#pragma once

#include <vector>
#include <memory>
#include "Entity/Entity.h"
#include "../../VectorGraphics.h"
#include "../../Rendering/Layer.h"
#include "../../Camera.h" // Ensure Camera is included
#include "../../Renderer.h"
#include <GLFW/glfw3.h> // Ensure GLFW is included

class Entities {
public:
    // Update constructor declaration
    Entities(Camera* cam, GLFWwindow* win);
    ~Entities();

    void update(float deltaTime);
    void render(bool batched = false);

    // Entity management
    size_t createEntity(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    void removeEntity(size_t index);
    Entity* getEntity(size_t index);
    const Entity* getEntity(size_t index) const;
    size_t getEntityCount() const { return entities.size(); }

    // Entity control
    void moveEntity(size_t index, const glm::vec2& target);
    void setEntityState(size_t index, EntityState state);

private:
    std::vector<std::unique_ptr<Entity>> entities;
    std::shared_ptr<Rendering::Layer> entityLayer;

    // Update methods
    void updateMovement(float deltaTime);
    void updateWork(float deltaTime);
};