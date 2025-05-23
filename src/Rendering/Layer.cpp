#include "Rendering/Layer.h"
#include "VectorGraphics.h"
#include <iostream>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include "ConfigManager.h"
#include <string>
#include <typeinfo> // Add for typeid
#include "../CoordinateSystem.h"

namespace Rendering {

// Update constructor definition to match new signature and initialize members
Layer::Layer(float zIndex, ProjectionType projType, Camera* cam, GLFWwindow* win)
    : zIndex(zIndex)
    , visible(true)
    , projectionType(projType) // Respect the provided type
    , camera(cam) // Initialize camera
    , window(win) // Initialize window
    , parent(nullptr)
    , childrenNeedSorting(false)
    {
}

void Layer::addItem(std::shared_ptr<Layer> item) {
    // If the child's projection type is the default (WorldSpace),
    // make it inherit the parent's projection type.
    // Otherwise, respect the type it was constructed with.
    if (item->getProjectionType() == ProjectionType::WorldSpace) {
        item->setProjectionType(this->projectionType);
    }
    
    // RE-ADD: Pass camera and window pointers directly to the child's members
    item->camera = this->camera;
    item->window = this->window;

    // Set parent pointer for the child
    item->setParent(this);

    // Optimization: Only mark for sorting if necessary
    if (!childrenNeedSorting) { // Only check if not already marked
        if (!children.empty()) {
            // If the new item's zIndex is less than the last item's zIndex,
            // it breaks the sort order, so mark for sorting.
            if (item->getZIndex() < children.back()->getZIndex()) {
                childrenNeedSorting = true;
            }
            // Otherwise (new item >= last item), it can be appended without sorting.
        } else {
            // Adding the first item doesn't require sorting the list of one.
            // childrenNeedSorting remains false.
        }
    }
    // If childrenNeedSorting was already true, it stays true.
    
    children.push_back(item);
    // We no longer unconditionally set childrenNeedSorting = true here.
}

void Layer::removeItem(std::shared_ptr<Layer> item) {
    auto it = std::find(children.begin(), children.end(), item);
    if (it == children.end()) {
        std::cerr << "Warning: Attempted to remove an item that is not present in the layer." << std::endl;
        return; // Item not found, exit the function
    }
    
    // Clear parent pointer before removing
    (*it)->setParent(nullptr);

    // Optimization: Check if removal could affect sort order.
    // If we remove anything other than the last element, the indices might shift,
    // but the relative order of remaining elements doesn't change.
    // If the list was sorted, it remains sorted after removal.
    // Therefore, removing an item doesn't require resorting.
    // We don't need to set childrenNeedSorting = true here.
    // bool removedFromEnd = (it == children.end() - 1);
    children.erase(it);
    // If we removed the last item, sorting is definitely not needed.
    // If we removed from the middle, the remaining items are still sorted relative to each other.
    // childrenNeedSorting = !removedFromEnd; // This logic is likely incorrect/unnecessary
}

void Layer::clearItems() {
    // Clear parent pointers before clearing children
    for (auto& child : children) {
        child->setParent(nullptr);
    }
    children.clear();
    // No children left, so no sorting needed
    childrenNeedSorting = false; 
}

// Add implementation for setZIndex
void Layer::setZIndex(float z) {
    if (zIndex != z) {
        zIndex = z;
        // If this layer has a parent, mark the parent as needing sorting
        if (parent) {
            parent->childrenNeedSorting = true;
        }
    }
}

// Add implementation for setParent
void Layer::setParent(Layer* p) {
    parent = p;
}

void Layer::sortChildren() {
    // Only sort if the flag is set
    if (childrenNeedSorting) {
        std::sort(children.begin(), children.end(),
            [](const std::shared_ptr<Layer>& a, const std::shared_ptr<Layer>& b) {
                return a->getZIndex() < b->getZIndex();
            }
        );
        // Reset the flag after sorting
        childrenNeedSorting = false; 
    }
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
    auto& coordSys = CoordinateSystem::getInstance();
    
    if (projectionType == ProjectionType::ScreenSpace) {
        // Create screen-space projection matrix for UI
        // (0,0) = top-left, Y increases downward
        return coordSys.createScreenSpaceProjection();
    } else if (camera != nullptr) {
        // For world space, get projection matrix from camera
        return camera->getProjectionMatrix();
    } else if (projectionType == ProjectionType::WorldSpace) {
        // Create world-space projection matrix
        // (0,0) = center, Y increases upward
        return coordSys.createWorldSpaceProjection();
    }
    
    // Default to identity if no camera or window is available
    return glm::mat4(1.0f);
}

void Layer::render(bool batched) {
    if (!visible) {
        return;
    }

    // Sort children if needed (now conditional)
    sortChildren(); 

    // Get matrices based on projection type
    glm::mat4 viewMatrix = getViewMatrix();
    glm::mat4 projectionMatrix = getProjectionMatrix();
    
    beginBatch();
    for (auto& child : children) {
        child->render(true); 
    }

    // Only render with matrices if this isn't part of a batch operation
    if (!batched) {
        VectorGraphics::getInstance().render(viewMatrix, projectionMatrix);
        endBatch();
        VectorGraphics::getInstance().clear();
    }
}

void Layer::beginBatch() {
    VectorGraphics::getInstance().beginBatch();
}

void Layer::endBatch() {
    VectorGraphics::getInstance().endBatch();
}

void Layer::update(float deltaTime) {
    // Skip if layer is not visible
    if (!visible) {
        return;
    }

    // Update all children
    for (auto& child : children) {
        child->update(deltaTime);
    }
}
void Layer::handleInput(float deltaTime) {

    // Skip if layer is not visible
    if (!visible) {
        return;
    }

    // Create a copy of the children list to iterate over.
    // This prevents iterator invalidation if a child removes itself
    // or other children from the original 'children' list during its handleInput() call.
    auto children_to_process = children; 

    // Propagate input handling to all children from the copy
    for (const auto& child : children_to_process) {
        // Note: For typeid(*child).name() to return the dynamic (actual) type of the child,
        // the Layer class must be polymorphic (e.g., have a virtual destructor).
        // The output of .name() is compiler-dependent and might be a mangled name.
        //std::cout << "Layer::handleInput: propagating to child (Type: " << typeid(*child).name() << ", Z: " << child->getZIndex() << ")" << std::endl;
        
        child->handleInput(deltaTime); // This call might modify the original 'this->children' list
    }
}

} // namespace Rendering