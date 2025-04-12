#pragma once

#include <glm/glm.hpp>
#include <cmath>

class Camera {
public:
    Camera();
    ~Camera() = default;
    
    void setPosition(const glm::vec3& position);
    void setTarget(const glm::vec3& target);
    void setUp(const glm::vec3& up);
    void setOrthographicProjection(float left, float right, float bottom, float top, float near, float far);
    void setPerspectiveProjection(float fov, float aspect, float near, float far);
    
    void move(const glm::vec3& offset);
    void rotate(float angle, const glm::vec3& axis);
    void zoom(float amount);
    
    const glm::mat4& getViewMatrix() const;
    const glm::mat4& getProjectionMatrix() const;
    const glm::vec3& getPosition() const;
    const glm::vec3& getTarget() const;

    glm::vec3 screenToWorld(const glm::vec3& screenPos) const;

    float getAspectRatio() const {
        // Calculate the width-to-height ratio, taking absolute values to handle negative values
        float width = std::abs(projectionRight - projectionLeft);
        float height = std::abs(projectionTop - projectionBottom);
        return width / height;
    }

private:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    
    float projectionLeft = -1.0f;
    float projectionRight = 1.0f;
    float projectionBottom = -1.0f;
    float projectionTop = 1.0f;
    
    void updateViewMatrix();
}; 