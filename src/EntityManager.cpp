#include "EntityManager.h"
#include "Entity.h"
#include "VectorGraphics.h"
#include <algorithm>
#include <cmath>
#include <iostream>

EntityManager::EntityManager() {
    // Create entity layer with WorldSpace projection
    // Camera, window, and renderer will be set later
    entityLayer = std::make_shared<Rendering::Layer>(100.0f, Rendering::ProjectionType::WorldSpace);
}

EntityManager::~EntityManager() = default;

void EntityManager::update(float deltaTime) {
    //std::cout << "Updating " << entities.size() << " entities" << std::endl;
    updateMovement(deltaTime);
    updateWork(deltaTime);
}

void EntityManager::render(VectorGraphics& graphics) {
    //std::cout << "Rendering " << entities.size() << " entities" << std::endl;
    for (const auto& entity : entities) {
        if (entity) {
            //std::cout << "Rendering entity at: (" << entity->getPosition().x << ", " << entity->getPosition().y << ")" << std::endl;
            entity->render(graphics);
        }
    }
    
    // Let the entity layer handle rendering
    // No need for finalization here since World handles the final rendering step
    entityLayer->render(graphics);
}

size_t EntityManager::createEntity(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
    entities.push_back(std::make_unique<Entity>(position, size, color));
    
    // Get the newly created entity
    Entity* entity = entities.back().get();
    
    // No need to set camera, window, and renderer since Entity doesn't have these methods
    // Just log that a new entity has been created
    std::cout << "Created new entity at position: (" << position.x << ", " << position.y << ")" << std::endl;
    
    return entities.size() - 1;
}

void EntityManager::removeEntity(size_t index) {
    if (index < entities.size()) {
        entities[index] = nullptr;
    }
}

Entity* EntityManager::getEntity(size_t index) {
    if (index < entities.size()) {
        return entities[index].get();
    }
    return nullptr;
}

const Entity* EntityManager::getEntity(size_t index) const {
    if (index < entities.size()) {
        return entities[index].get();
    }
    return nullptr;
}

void EntityManager::moveEntity(size_t index, const glm::vec2& target) {
    if (index < entities.size() && entities[index]) {
        std::cout << "Moving entity " << index << " to position: (" << target.x << ", " << target.y << ")" << std::endl;
        entities[index]->setTargetPosition(target);
        entities[index]->setState(EntityState::MOVING);
    } else {
        std::cerr << "Invalid entity index: " << index << std::endl;
    }
}

void EntityManager::setEntityState(size_t index, EntityState state) {
    if (index < entities.size() && entities[index]) {
        std::cout << "Setting entity " << index << " state to: " << static_cast<int>(state) << std::endl;
        entities[index]->setState(state);
    } else {
        std::cerr << "Invalid entity index: " << index << std::endl;
    }
}

void EntityManager::updateMovement(float deltaTime) {
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

void EntityManager::updateWork(float deltaTime) {
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