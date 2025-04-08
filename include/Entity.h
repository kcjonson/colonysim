#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <memory>
#include "VectorGraphics.h"

enum class EntityType {
    WORKER,
    BUILDING,
    RESOURCE
};

enum class EntityState {
    IDLE,
    MOVING,
    WORKING,
    GATHERING
};

class Entity {
public:
    Entity(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
    ~Entity() = default;

    void update(float deltaTime);
    void render(VectorGraphics& graphics);

    // Getters and setters
    EntityType getType() const { return type; }
    EntityState getState() const { return state; }
    const glm::vec2& getPosition() const { return position; }
    const glm::vec2& getSize() const { return size; }
    const glm::vec2& getTargetPosition() const { return targetPosition; }
    float getRotation() const { return rotation; }
    float getSpeed() const { return speed; }
    float getHealth() const { return health; }
    float getWorkProgress() const { return workProgress; }
    const std::string& getName() const { return name; }
    const std::vector<std::pair<std::string, int>>& getInventory() const { return inventory; }

    void setType(EntityType newType) { type = newType; }
    void setState(EntityState newState) { state = newState; }
    void setPosition(const glm::vec2& newPosition) { position = newPosition; }
    void setSize(const glm::vec2& newSize) { size = newSize; }
    void setTargetPosition(const glm::vec2& newTarget) { targetPosition = newTarget; }
    void setRotation(float newRotation) { rotation = newRotation; }
    void setSpeed(float newSpeed) { speed = newSpeed; }
    void setHealth(float newHealth) { health = newHealth; }
    void setWorkProgress(float newProgress) { workProgress = newProgress; }
    void setName(const std::string& newName) { name = newName; }

private:
    EntityType type;
    EntityState state;
    glm::vec2 position;
    glm::vec2 size;
    glm::vec2 targetPosition;
    float rotation;
    float speed;
    float health;
    float workProgress;
    std::string name;
    std::vector<std::pair<std::string, int>> inventory;
    glm::vec4 color;
}; 