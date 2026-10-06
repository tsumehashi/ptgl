#include "ObjectScene.h"
#include "GraphicsView.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace ptgl {
SceneObject::SceneObject(const std::string& name, const Eigen::Vector3d& position, DrawFunction draw)
    : transform_(std::make_shared<Transform>(position, Eigen::Matrix3d::Identity())),
      draw_(std::move(draw)), lastPosition_(position), lastRotation_(Eigen::Matrix3d::Identity())
{
    if (!position.allFinite()) throw std::invalid_argument("SceneObject position must be finite");
    setName(name);
}

void SceneObject::setDrawFunction(DrawFunction draw)
{
    draw_ = std::move(draw);
    notifyGeometryChanged();
}

void SceneObject::notifyGeometryChanged()
{
    if (graphicsWindow()) graphicsWindow()->notifySceneChanged();
}

void SceneObject::renderScene(Renderer3D* r)
{
    r->pushMatrix();
    try {
        r->transform(transform_->position(), transform_->rotation());
        if (draw_) draw_(r);
    } catch (...) {
        r->popMatrix();
        throw;
    }
    r->popMatrix();
}

ObjectScene::ObjectScene(GraphicsView& view) : view_(view), handle_(std::make_shared<handle::TransformHandle>())
{
    idleTransform_ = handle_->transform();
    handle_->setEnabled(false);
    view_.addGraphicsItem(handle_);
}

ObjectScene::~ObjectScene()
{
    if (selected_) selected_->setOpacity(originalOpacity_);
    handle_->setEnabled(false);
    handle_->setTransform(idleTransform_);
}

SceneObjectPtr ObjectScene::addObject(const std::string& name, const Eigen::Vector3d& position,
                                     SceneObject::DrawFunction draw)
{
    auto object = std::make_shared<SceneObject>(name, position, std::move(draw));
    objects_.push_back(object);
    view_.addGraphicsItem(object);
    return object;
}

bool ObjectScene::removeObject(const SceneObjectPtr& object)
{
    // Copy: callers may pass a reference into objects_ or selected_.
    auto target = object;
    if (!target || std::find(objects_.begin(), objects_.end(), target) == objects_.end()) return false;
    view_.removeGraphicsItem(target); // Also notifies itemRemoved().
    return true;
}

bool ObjectScene::removeSelectedObject() { return removeObject(selected_); }

void ObjectScene::clear()
{
    const auto objects = objects_;
    for (const auto& object : objects) removeObject(object);
}

void ObjectScene::selectObject(const SceneObjectPtr& object)
{
    auto target = object;
    if (target && (!editingEnabled_ || target->graphicsWindow() != &view_ || !target->isEnabled()
        || !target->isVisible() || !target->isPickable()
        || std::find(objects_.begin(), objects_.end(), target) == objects_.end()))
        throw std::invalid_argument("Object is not editable in this scene");
    if (target == selected_) return;
    view_.cancelGraphicsItemDrag();
    if (selected_) selected_->setOpacity(originalOpacity_);
    selected_ = target;
    handle_->setEnabled(bool(selected_));
    handle_->setTransform(selected_ ? selected_->transform() : idleTransform_);
    if (selected_) {
        originalOpacity_ = selected_->opacity();
        selected_->setOpacity(originalOpacity_ * selectionOpacity_);
    }
}

void ObjectScene::setSelectionOpacity(double opacity)
{
    if (!std::isfinite(opacity) || opacity < 0 || opacity > 1)
        throw std::invalid_argument("Selection opacity must be in [0,1]");
    selectionOpacity_ = opacity;
    if (selected_) selected_->setOpacity(originalOpacity_ * opacity);
}

void ObjectScene::setEditingEnabled(bool enabled)
{
    if (!enabled) selectObject(nullptr);
    if (enabled && handle_->graphicsWindow() != &view_) view_.addGraphicsItem(handle_);
    editingEnabled_ = enabled;
}

void ObjectScene::pick(const GraphicsItemPtr& item)
{
    if (!editingEnabled_) return;
    for (auto p = item.get(); p; p = p->parentItem()) {
        if (p == handle_.get()) return;
        auto found = std::find_if(objects_.begin(), objects_.end(),
                                 [&](const SceneObjectPtr& object) { return object.get() == p; });
        if (found != objects_.end()) {
            const auto& object = *found;
            selectObject(object->isEnabled() && object->isVisible() && object->isPickable()
                         && object->graphicsWindow() == &view_ ? object : nullptr);
            return;
        }
    }
    selectObject(nullptr);
}

void ObjectScene::update()
{
    if (selected_ && (!selected_->isEnabled() || !selected_->isVisible() || !selected_->isPickable()))
        selectObject(nullptr);
    bool changed = false;
    for (auto& object : objects_) {
        const auto& t = object->transform();
        if (object->lastPosition_ != t->position() || object->lastRotation_ != t->rotation()) changed = true;
        object->lastPosition_ = t->position();
        object->lastRotation_ = t->rotation();
    }
    if (changed) view_.notifySceneChanged();
}

void ObjectScene::itemRemoved(const GraphicsItemPtr& item)
{
    if (item == handle_) { selectObject(nullptr); editingEnabled_ = false; }
    if (item == selected_) selectObject(nullptr);
    objects_.erase(std::remove(objects_.begin(), objects_.end(), item), objects_.end());
}
} // namespace ptgl
