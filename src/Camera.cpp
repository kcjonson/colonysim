#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Camera::Camera() : position(0.0f, 0.0f, 5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f) {
    // updateViewMatrix(); // Don't call here, let first getViewMatrix call handle it.
    viewMatrixDirty = true; // Ensure it's marked dirty initially
    // Set a larger initial view area for better visibility
    setOrthographicProjection(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f);
}

void Camera::setPosition(const glm::vec3& newPosition) {
    if (position != newPosition) {
        position = newPosition;
        viewMatrixDirty = true;
    }
}

void Camera::setTarget(const glm::vec3& newTarget) {
    if (target != newTarget) {
        target = newTarget;
        viewMatrixDirty = true;
    }
}

void Camera::setUp(const glm::vec3& newUp) {
    if (up != newUp) {
        up = newUp;
        viewMatrixDirty = true;
    }
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
    if (offset != glm::vec3(0.0f)) { // Only mark dirty if actually moving
        position += offset;
        target += offset; // Assuming target moves with position
        viewMatrixDirty = true;
    }
}

void Camera::rotate(float angle, const glm::vec3& axis) {
    // Rotation always changes the view
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, axis);
    position = glm::vec3(rotation * glm::vec4(position, 1.0f));
    viewMatrixDirty = true;
    // updateViewMatrix(); // Defer update until matrix is requested
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
    if (viewMatrixDirty) {
        updateViewMatrix(); // Update if dirty
    }
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
    // This now correctly uses the potentially updated view matrix via getViewMatrix()
    glm::mat4 inverseVP = glm::inverse(projectionMatrix * getViewMatrix());
    glm::vec4 worldPos = inverseVP * glm::vec4(screenPos, 1.0f);
    if (worldPos.w == 0.0f) return glm::vec3(0.0f); // Avoid division by zero
    return glm::vec3(worldPos) / worldPos.w;
}

void Camera::updateViewMatrix() const {
    viewMatrix = glm::lookAt(position, target, up);
    viewMatrixDirty = false; // Mark as clean
}
