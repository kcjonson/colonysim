#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Camera::Camera() : position(0.0f, 0.0f, 5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f) {
    updateViewMatrix();
    // Set a larger initial view area for better visibility
    setOrthographicProjection(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
}

void Camera::setPosition(const glm::vec3& position) {
    this->position = position;
    updateViewMatrix();
}

void Camera::setTarget(const glm::vec3& target) {
    this->target = target;
    updateViewMatrix();
}

void Camera::setUp(const glm::vec3& up) {
    this->up = up;
    updateViewMatrix();
}

void Camera::setOrthographicProjection(float left, float right, float bottom, float top, float near, float far) {
    projectionLeft = left;
    projectionRight = right;
    projectionBottom = bottom;
    projectionTop = top;
    projectionMatrix = glm::ortho(left, right, bottom, top, near, far);
}

void Camera::setPerspectiveProjection(float fov, float aspect, float near, float far) {
    projectionMatrix = glm::perspective(glm::radians(fov), aspect, near, far);
}

void Camera::move(const glm::vec3& offset) {
    position += offset;
    target += offset;
    updateViewMatrix();
}

void Camera::rotate(float angle, const glm::vec3& axis) {
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, axis);
    position = glm::vec3(rotation * glm::vec4(position, 1.0f));
    updateViewMatrix();
}

void Camera::zoom(float amount) {
    // Instead of moving the camera position, we'll directly adjust the orthographic projection
    // This is a simpler and more reliable approach for 2D zooming
    
    // Get the current width and height of the view frustum
    float currentWidth = projectionRight - projectionLeft;
    float currentHeight = projectionTop - projectionBottom;
    
    // Calculate scale factor based on zoom direction
    // For zoom in (amount > 0), we want to reduce size (scaleFactor < 1)
    // For zoom out (amount < 0), we want to increase size (scaleFactor > 1)
    float scaleFactor = 1.0f - (amount * 0.01f);
    
    // TODO: Renable this. It was blocking zooming out.
    // Ensure we never zoom in too close or zoom out too far
    // if ((amount > 0 && currentWidth < 0.5f) || (amount < 0 && currentWidth > 200.0f)) {
    //     return; // Prevent extreme zoom levels
    // }
    
    // Calculate new dimensions
    float newWidth = currentWidth * scaleFactor;
    float newHeight = currentHeight * scaleFactor;
    
    // Calculate new projection bounds, keeping the center point the same
    float centerX = (projectionLeft + projectionRight) * 0.5f;
    float centerY = (projectionBottom + projectionTop) * 0.5f;
    
    float newLeft = centerX - newWidth * 0.5f;
    float newRight = centerX + newWidth * 0.5f;
    float newBottom = centerY - newHeight * 0.5f;
    float newTop = centerY + newHeight * 0.5f;
    
    // Apply the new projection 
    // Debug output for zoom events
    std::cout << "Camera zoom: " << amount << std::endl;
    std::cout << "Projection bounds: " << newLeft << ", " << newRight << ", " << newBottom << ", " << newTop << std::endl;
    setOrthographicProjection(newLeft, newRight, newBottom, newTop, 0.1f, 100.0f);
}

const glm::mat4& Camera::getViewMatrix() const {
    return viewMatrix;
}

const glm::mat4& Camera::getProjectionMatrix() const {
    return projectionMatrix;
}

const glm::vec3& Camera::getPosition() const {
    return position;
}

const glm::vec3& Camera::getTarget() const {
    return target;
}

glm::vec3 Camera::screenToWorld(const glm::vec3& screenPos) const {
    glm::mat4 inverseVP = glm::inverse(projectionMatrix * viewMatrix);
    glm::vec4 worldPos = inverseVP * glm::vec4(screenPos, 1.0f);
    return glm::vec3(worldPos) / worldPos.w;
}

void Camera::updateViewMatrix() {
    viewMatrix = glm::lookAt(position, target, up);
}
