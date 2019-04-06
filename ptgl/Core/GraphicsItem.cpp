#include "GraphicsItem.h"
#include <algorithm>
#include "GraphicsView.h"
#include "Renderer3D.h"
#include "Renderer2D.h"
#include "TextRenderer.h"

namespace ptgl {

GraphicsItem::GraphicsItem()
{

}

GraphicsItem::~GraphicsItem()
{
    for (auto ptr : children_) {
        ptr->parent_ = nullptr;
        ptr->setGraphicsView(nullptr);
    }
}

void GraphicsItem::addChild(GraphicsItemPtr item)
{
    if (item->parentItem()) {
        item->parentItem()->removeChild(item);
    }
    item->parent_ = this;
    GraphicsItem::traverse(item, [&](GraphicsItemPtr ptr){
        ptr->setGraphicsView(this->graphicsView_);
    });
    children_.push_back(item);
}

void GraphicsItem::removeChild(GraphicsItemPtr item)
{
    auto itr = std::find(children_.begin(), children_.end(), item);
    if (itr != children_.end()) {
        (*itr)->parent_ = nullptr;
        GraphicsItem::traverse(*itr, [&](GraphicsItemPtr ptr){
            ptr->setGraphicsView(nullptr);
        });
        children_.erase(itr);
    }
}

void GraphicsItem::setVisible(bool visible)
{
    visible_ = visible;
    for (auto ptr : children_) {
        ptr->setVisible(visible);
    }
}

void GraphicsItem::setEnabled(bool enabled)
{
    enabled_ = enabled;
    for (auto ptr : children_) {
        ptr->setEnabled(enabled);
    }
}

void GraphicsItem::setPickable(bool enable)
{
    pickable_ = enable;
    for (auto ptr : children_) {
        ptr->setPickable(enable);
    }
}

void GraphicsItem::setEnabledWheelEvent(bool enable)
{
    enabledWheelEvent_ = enable;
}

void GraphicsItem::setEnabledKeyEvent(bool enable)
{
    enabledKeyEvent_ = enable;
#if 0
    for (auto ptr : children_) {
        ptr->setEnabledKeyEvent(enable);
    }
#endif
}

int GraphicsItem::viewWidth() const
{
    return graphicsView_ ? graphicsView_->width() : 0;
}

int GraphicsItem::viewHeight() const
{
    return graphicsView_ ? graphicsView_->height() : 0;
}

void GraphicsItem::setHoverd(bool hoverd)
{
    hoverd_ = hoverd;
}

void GraphicsItem::setPicked(bool picked)
{
    if (picked_ != picked) {
        pickChanged_ = true;
    }
    picked_ = picked;
}

void GraphicsItem::setChecked(bool checked)
{
    if (checked_ != checked) {
        checked_ = checked;
        checkChanged_ = true;
    }
}

void GraphicsItem::setGraphicsView(GraphicsView* view)
{
    if (!graphicsView_ && view)    {
        // attach
        attachedGraphicsWindow(view);
    } else if (graphicsView_ && !view) {
        // detach
        detachedGraphicsWindow(graphicsView_);
    }

    graphicsView_ = view;
}

// Mouse event
void GraphicsItem::executeMousePressEvent(GraphicsItemMouseEvent* e)
{
    setChecked(!checked_);
    pressed_ = true;
    mousePressEvent(e);
}

void GraphicsItem::executeMouseMoveEvent(ptgl::GraphicsItemMouseEvent* e)
{
    mouseMoveEvent(e);
}

void GraphicsItem::executeMouseReleaseEvent(GraphicsItemMouseEvent* e)
{
    pressed_ = false;
    mouseReleaseEvent(e);
}

// Prev/Post event
void GraphicsItem::executePrevProcess()
{
    prevProcess();
}

void GraphicsItem::executePostProcess()
{
    postProcess();
    pickChanged_ = false;
    checkChanged_ = false;
}

void GraphicsItem::traverse(GraphicsItemPtr item, std::function<void (GraphicsItemPtr)> func)
{
    if (!item) return;

    func(item);
    for (auto ptr : item->children_) {
        traverse(ptr, func);
    }
}

void GraphicsItem::traverseReverse(GraphicsItemPtr item, std::function<void (GraphicsItemPtr)> func)
{
    if (!item) return;

    for (auto ptr : item->children_) {
        traverse(ptr, func);
    }
    func(item);
}

} /* namespace ptgl */
