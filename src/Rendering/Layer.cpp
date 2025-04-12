#include "Rendering/Layer.h"
#include "VectorGraphics.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Rendering {

Layer::Layer(float zIndex, ProjectionType projType)
    : zIndex(zIndex)
    , visible(true)
    , projectionType(projType)
    , camera(nullptr)
    , window(nullptr)
    , renderer(nullptr) {
}

void Layer::addItem(std::shared_ptr<Layer> item) {
    // If the child doesn't have its own projectionType set, inherit from parent
    if (item->getProjectionType() != projectionType) {
        item->setProjectionType(projectionType);
    }
    
    // Pass camera and window to the child
    if (camera != nullptr) {
        item->setCamera(camera);
    }
    
    if (window != nullptr) {
        item->setWindow(window);
    }
    
    // Pass renderer to the child
    if (renderer != nullptr) {
        item->setRenderer(renderer);
    }
    
    children.push_back(item);
    // Sort children after adding a new item
    sortChildren();
}

void Layer::removeItem(std::shared_ptr<Layer> item) {
    auto it = std::find(children.begin(), children.end(), item);
    if (it == children.end()) {
        std::cerr << "Warning: Attempted to remove an item that is not present in the layer." << std::endl;
        return; // Item not found, exit the function
    }
    
    children.erase(it);
}

void Layer::clearItems() {
    children.clear();
}

void Layer::sortChildren() {
    std::sort(children.begin(), children.end(),
        [](const std::shared_ptr<Layer>& a, const std::shared_ptr<Layer>& b) {
            return a->getZIndex() < b->getZIndex();
        }
    );
}

glm::mat4 Layer::getViewMatrix() const {
    if (projectionType == ProjectionType::ScreenSpace) {
        // For screen space, always use identity view matrix
        return glm::mat4(1.0f);
    } else if (camera != nullptr) {
        // For world space, get view matrix from camera
        return camera->getViewMatrix();
    } else if (projectionType == ProjectionType::WorldSpace) {
        // Create a default view matrix if no camera is available
        // Default to looking at the origin from a slightly offset position
        glm::vec3 eye(0.0f, 0.0f, 100.0f);  // Camera position
        glm::vec3 center(0.0f, 0.0f, 0.0f); // Look at origin
        glm::vec3 up(0.0f, 1.0f, 0.0f);     // Up vector
        
        // Calculate look-at matrix
        glm::vec3 f = glm::normalize(center - eye);
        glm::vec3 s = glm::normalize(glm::cross(f, up));
        glm::vec3 u = glm::cross(s, f);
        
        glm::mat4 viewMatrix(1.0f);
        viewMatrix[0][0] = s.x;
        viewMatrix[1][0] = s.y;
        viewMatrix[2][0] = s.z;
        viewMatrix[0][1] = u.x;
        viewMatrix[1][1] = u.y;
        viewMatrix[2][1] = u.z;
        viewMatrix[0][2] = -f.x;
        viewMatrix[1][2] = -f.y;
        viewMatrix[2][2] = -f.z;
        viewMatrix[3][0] = -glm::dot(s, eye);
        viewMatrix[3][1] = -glm::dot(u, eye);
        viewMatrix[3][2] = glm::dot(f, eye);
        
        return viewMatrix;
    }
    
    // Default to identity if no camera is available
    return glm::mat4(1.0f);
}

glm::mat4 Layer::getProjectionMatrix() const {
    if (projectionType == ProjectionType::ScreenSpace && window != nullptr) {
        // Create screen-space projection matrix for UI
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        
        glm::mat4 screenProjection = glm::mat4(1.0f);
        screenProjection[0][0] = 2.0f / width;       // Scale x
        screenProjection[1][1] = -2.0f / height;     // Scale y (negative to flip y-axis)
        screenProjection[3][0] = -1.0f;              // Translate x
        screenProjection[3][1] = 1.0f;               // Translate y
        
        return screenProjection;
    } else if (camera != nullptr) {
        // For world space, get projection matrix from camera
        return camera->getProjectionMatrix();
    } else if (projectionType == ProjectionType::WorldSpace && window != nullptr) {
        // Create a default world space projection if no camera is available
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        
        // Default to orthographic projection
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        float viewHeight = 1000.0f; // Default view height
        float viewWidth = viewHeight * aspectRatio;
        
        // Create orthographic projection matrix
        glm::mat4 orthoProjection = glm::mat4(1.0f);
        orthoProjection[0][0] = 2.0f / viewWidth;
        orthoProjection[1][1] = 2.0f / viewHeight;
        orthoProjection[2][2] = -2.0f / 1000.0f;     // Near/far planes at 0 and 1000
        orthoProjection[3][0] = 0.0f;                // No X translation (centered)
        orthoProjection[3][1] = 0.0f;                // No Y translation (centered)
        orthoProjection[3][2] = -1.0f;               // Z translation to fit near/far into -1 to 1 range
        
        return orthoProjection;
    }
    
    // Default to identity if no camera or window is available
    return glm::mat4(1.0f);
}

void Layer::render(VectorGraphics& graphics) {
    if (!visible) {
        return;
    }

    // Get matrices based on projection type
    glm::mat4 viewMatrix = getViewMatrix();
    glm::mat4 projectionMatrix = getProjectionMatrix();
    
    // Forward to renderWithMatrices
    renderWithMatrices(graphics, viewMatrix, projectionMatrix);
}

void Layer::renderWithMatrices(VectorGraphics& graphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (!visible) {
        return;
    }

    // Sort children if needed
    sortChildren();

    // Render all children
    for (auto& child : children) {
        // Each child gets rendered with the layer's matrices
        child->renderWithMatrices(graphics, viewMatrix, projectionMatrix);
    }
}

void Layer::renderScreenSpace(VectorGraphics& graphics, const glm::mat4& projectionMatrix) {
    if (!visible) return;

    // Sort children if needed
    sortChildren();

    // For screen space, we always use identity view matrix
    glm::mat4 identityView = glm::mat4(1.0f);

    // Render all children
    for (auto& child : children) {
        child->renderWithMatrices(graphics, identityView, projectionMatrix);
    }
}

void Layer::beginBatch(VectorGraphics& graphics) {
    graphics.beginBatch();
}

void Layer::endBatch(VectorGraphics& graphics) {
    graphics.endBatch();
}

void Layer::finalizeRender(VectorGraphics& graphics) {
    // Get matrices based on projection type
    glm::mat4 viewMatrix = getViewMatrix();
    glm::mat4 projectionMatrix = getProjectionMatrix();
    
    // If we have a renderer, set the view and projection
    if (renderer) {
        renderer->setView(viewMatrix);
        renderer->setProjection(projectionMatrix);
    }
    
    // Call VectorGraphics.render with appropriate matrices
    graphics.render(viewMatrix, projectionMatrix);
}

} // namespace Rendering 