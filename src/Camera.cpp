#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Camera::Camera() : position(0.0f, 0.0f, 5.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f) {
    updateViewMatrix();
    setOrthographicProjection(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
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
    position += glm::normalize(target - position) * amount;
    updateViewMatrix();
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
