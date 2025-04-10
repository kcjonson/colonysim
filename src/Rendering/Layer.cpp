#include "Rendering/Layer.h"
#include "VectorGraphics.h"
#include <iostream>

namespace Rendering {

Layer::Layer(float zIndex)
    : zIndex(zIndex)
    , visible(true) {
}

void Layer::addItem(std::shared_ptr<Layer> item) {
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

void Layer::render(VectorGraphics& graphics, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    //std::cout << "Layer children count: " << children.size() << std::endl;
    if (!visible) {
        return;
    }

    // Sort children if needed
    sortChildren();

    // Render all children
    for (auto& child : children) {
        child->render(graphics, viewMatrix, projectionMatrix);
    }
}

void Layer::renderScreenSpace(VectorGraphics& graphics, const glm::mat4& projectionMatrix) {
    if (!visible) return;

    // Sort children if needed
    sortChildren();

    // Render all children
    for (auto& child : children) {
        child->renderScreenSpace(graphics, projectionMatrix);
    }
}

void Layer::beginBatch(VectorGraphics& graphics) {
    graphics.beginBatch();
}

void Layer::endBatch(VectorGraphics& graphics) {
    graphics.endBatch();
}

} // namespace Rendering 