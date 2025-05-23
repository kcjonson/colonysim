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
    
    const glm::mat4& getViewMatrix() const; // Keep const
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
    
    float getProjectionLeft() const { return projectionLeft; }
    float getProjectionRight() const { return projectionRight; }
    float getProjectionBottom() const { return projectionBottom; }
    float getProjectionTop() const { return projectionTop; }

private:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    mutable glm::mat4 viewMatrix; // Mutable for caching in const method
    glm::mat4 projectionMatrix;
    
    float projectionLeft = -1.0f;
    float projectionRight = 1.0f;
    float projectionBottom = -1.0f;
    float projectionTop = 1.0f;
    
    mutable bool viewMatrixDirty = true; // Mutable flag for caching
    void updateViewMatrix() const; // Keep const
};