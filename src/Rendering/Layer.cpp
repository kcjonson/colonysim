#include "Rendering/Layer.h"
#include "VectorGraphics.h"

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
    children.erase(
        std::remove_if(children.begin(), children.end(),
            [&item](const std::shared_ptr<Layer>& child) {
                return child == item;
            }
        ),
        children.end()
    );
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
    if (!visible) return;

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