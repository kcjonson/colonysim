#include "Entities.h"
#include "Entity/Entity.h"
#include "../../VectorGraphics.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// Constructor now takes camera and window pointers
Entities::Entities(Camera* cam, GLFWwindow* win) 
    : entityLayer(std::make_shared<Rendering::Layer>(150.0f, Rendering::ProjectionType::WorldSpace, cam, win)) {
    // Constructor body can be empty if initialization is done in the list
}

Entities::~Entities() = default;

void Entities::update(float deltaTime) {
    updateMovement(deltaTime);
    updateWork(deltaTime);
}

void Entities::render(bool batched) {
    // Let the entity layer handle rendering
    entityLayer->render(true);
}

size_t Entities::createEntity(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
    entities.push_back(std::make_unique<Entity>(position, size, color));
    
    // Get the newly created entity
    Entity* entity = entities.back().get();
    
    std::cout << "Created new entity at position: (" << position.x << ", " << position.y << ")" << std::endl;
    
    return entities.size() - 1;
}

void Entities::removeEntity(size_t index) {
    if (index < entities.size()) {
        entities[index] = nullptr;
    }
}

Entity* Entities::getEntity(size_t index) {
    if (index < entities.size()) {
        return entities[index].get();
    }
    return nullptr;
}

const Entity* Entities::getEntity(size_t index) const {
    if (index < entities.size()) {
        return entities[index].get();
    }
    return nullptr;
}

void Entities::moveEntity(size_t index, const glm::vec2& target) {
    if (index < entities.size() && entities[index]) {
        std::cout << "Moving entity " << index << " to position: (" << target.x << ", " << target.y << ")" << std::endl;
        entities[index]->setTargetPosition(target);
        entities[index]->setState(EntityState::MOVING);
    } else {
        std::cerr << "Invalid entity index: " << index << std::endl;
    }
}

void Entities::setEntityState(size_t index, EntityState state) {
    if (index < entities.size() && entities[index]) {
        std::cout << "Setting entity " << index << " state to: " << static_cast<int>(state) << std::endl;
        entities[index]->setState(state);
    } else {
        std::cerr << "Invalid entity index: " << index << std::endl;
    }
}

void Entities::updateMovement(float deltaTime) {
    for (size_t i = 0; i < entities.size(); ++i) {
        if (entities[i] && entities[i]->getState() == EntityState::MOVING) {
            glm::vec2 currentPos = entities[i]->getPosition();
            glm::vec2 targetPos = entities[i]->getTargetPosition();
            float distance = glm::distance(currentPos, targetPos);
            
            if (distance > 0.1f) {
                glm::vec2 direction = glm::normalize(targetPos - currentPos);
                float moveDistance = entities[i]->getSpeed() * deltaTime;
                glm::vec2 newPos = currentPos + direction * moveDistance;
                entities[i]->setPosition(newPos);
                
                // Update rotation to face movement direction
                float angle = std::atan2(direction.y, direction.x);
                entities[i]->setRotation(angle);
            } else {
                entities[i]->setPosition(targetPos);
                entities[i]->setState(EntityState::IDLE);
                std::cout << "Entity " << i << " reached target position" << std::endl;
            }
        }
    }
}

void Entities::updateWork(float deltaTime) {
    for (size_t i = 0; i < entities.size(); ++i) {
        if (entities[i] && entities[i]->getState() == EntityState::WORKING) {
            float progress = entities[i]->getWorkProgress() + deltaTime;
            entities[i]->setWorkProgress(progress);
            
            if (progress >= 1.0f) {
                entities[i]->setState(EntityState::IDLE);
                std::cout << "Entity " << i << " completed work" << std::endl;
            }
        }
    }
}