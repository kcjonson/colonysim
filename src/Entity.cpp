#include "Entity.h"
#include <cmath>

Entity::Entity(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    : type(EntityType::WORKER)
    , state(EntityState::IDLE)
    , position(position)
    , size(size)
    , targetPosition(position)
    , rotation(0.0f)
    , speed(50.0f)
    , health(1.0f)
    , workProgress(0.0f)
    , name("Entity")
    , color(color)
{
}

void Entity::update(float deltaTime) {
    if (state == EntityState::MOVING) {
        glm::vec2 direction = targetPosition - position;
        float distance = glm::length(direction);

        if (distance > 0.1f) {
            direction = glm::normalize(direction);
            glm::vec2 movement = direction * speed * deltaTime;

            if (glm::length(movement) >= distance) {
                position = targetPosition;
                state = EntityState::IDLE;
            } else {
                position += movement;
            }

            // Update rotation to face movement direction
            rotation = std::atan2(direction.y, direction.x);
        } else {
            position = targetPosition;
            state = EntityState::IDLE;
        }
    } else if (state == EntityState::WORKING) {
        workProgress += deltaTime * 0.5f; // Work at 50% speed
        if (workProgress >= 1.0f) {
            workProgress = 0.0f;
            state = EntityState::IDLE;
        }
    }
}

void Entity::render(VectorGraphics& graphics) {
    // Draw entity
    graphics.drawRectangle(
        position - size * 0.5f,
        size,
        color
    );

    // Draw health bar if entity has health
    if (health < 1.0f) {
        float healthBarWidth = size.x;
        float healthBarHeight = 2.0f;
        glm::vec4 healthColor(1.0f - health, health, 0.0f, 1.0f);
        graphics.drawRectangle(
            position - glm::vec2(healthBarWidth * 0.5f, size.y * 0.5f + healthBarHeight),
            glm::vec2(healthBarWidth * health, healthBarHeight),
            healthColor
        );
    }

    // Draw work progress if entity is working
    if (state == EntityState::WORKING && workProgress > 0.0f) {
        float progressBarWidth = size.x;
        float progressBarHeight = 2.0f;
        glm::vec4 progressColor(0.0f, 1.0f, 0.0f, 1.0f);
        graphics.drawRectangle(
            position - glm::vec2(progressBarWidth * 0.5f, size.y * 0.5f + progressBarHeight * 2.0f),
            glm::vec2(progressBarWidth * workProgress, progressBarHeight),
            progressColor
        );
    }
} 