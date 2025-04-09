#include "Rendering/Layer.h"
#include "VectorGraphics.h"

namespace Rendering {

Layer::Layer(float zIndex)
    : zIndex(zIndex)
    , visible(true) {
}

void Layer::addLayer(std::shared_ptr<Layer> layer) {
    children.push_back(layer);
    // Sort children after adding a new layer
    sortChildren();
}

void Layer::removeLayer(std::shared_ptr<Layer> layer) {
    children.erase(
        std::remove_if(children.begin(), children.end(),
            [&layer](const std::shared_ptr<Layer>& child) {
                return child == layer;
            }
        ),
        children.end()
    );
}

void Layer::clearLayers() {
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