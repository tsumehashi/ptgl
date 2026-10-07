#include "GraphicsView.h"
#include "GraphicsDriver.h"
#include "ptgl/GUI/Widget.h"
#include "ptgl/GUI/Layout.h"
#include <algorithm>
namespace ptgl
{
GraphicsItemPtr GraphicsView::guiAt(int x, int y) const
{
    auto items = getTraversedGraphicsItems();
    for (bool popup : {true, false})
        for (auto it = items.rbegin(); it != items.rend(); ++it)
            if (auto w = dynamic_cast<gui::Widget *>(it->get());
                w && w->isPopupLayer() == popup && w->hitTest(x, y))
                return *it;
    return {};
}
void GraphicsView::updateGui()
{
    // Parents arrange their children first; no layout mutation during picking.
    auto items = getTraversedGraphicsItems();
    for (auto &item : items)
        if (auto w = dynamic_cast<gui::Widget *>(item.get()); w && w->isVisible())
            w->layout();
    auto valid = [&](const GraphicsItemPtr &p) {
        return p && p->graphicsWindow() == this && p->isVisible() && p->isEnabled() && p->isPickable();
    };
    if (focusedGraphicsItem_ && !valid(focusedGraphicsItem_))
        setKeyboardFocus(nullptr);
    if (mouseGraphicsItem_ && !valid(mouseGraphicsItem_))
        cancelGraphicsItemDrag();
}
void GraphicsView::setKeyboardFocus(const GraphicsItemPtr &item)
{
    auto target = item;
    if (target && (target->graphicsWindow() != this || !target->isVisible() || !target->isEnabled() ||
                   !target->isPickable()))
        return;
    if (target == focusedGraphicsItem_)
        return;
    if (driver_)
        driver_->cancelTextComposition();
    auto old = std::move(focusedGraphicsItem_);
    if (old) {
        old->setPicked(false);
        old->selectLeaveEvent(graphicsItemSelectEvent_.get());
    }
    focusedGraphicsItem_ = target;
    if (target) {
        target->setPicked(true);
        target->selectEnterEvent(graphicsItemSelectEvent_.get());
        if (auto w = dynamic_cast<gui::Widget *>(target.get()); w && !w->isPopupLayer())
            for (auto p = w->parentItem(); p; p = p->parentItem())
                if (auto scroll = dynamic_cast<gui::ScrollArea *>(p))
                    scroll->ensureVisible(*w);
    }
}
void GraphicsView::clearItemInput(const GraphicsItemPtr &item)
{
    auto belongs = [&](const GraphicsItemPtr &c) {
        for (auto p = c.get(); p; p = p->parentItem())
            if (p == item.get())
                return true;
        return false;
    };
    if (belongs(mouseGraphicsItem_))
        cancelGraphicsItemDrag();
    if (belongs(focusedGraphicsItem_))
        setKeyboardFocus(nullptr);
    if (belongs(hoveredGraphicsItem_)) {
        hoveredGraphicsItem_->setHoverd(false);
        hoveredGraphicsItem_.reset();
    }
    if (belongs(pickedGraphicsItem_))
        pickedGraphicsItem_.reset();
    if (belongs(prevMousePressGraphicsItem_))
        prevMousePressGraphicsItem_.reset();
    for (auto &p : pickIdToItemList_)
        if (belongs(p))
            p.reset();
    if (belongs(pickingEvent_->pickedGraphicsItem()))
        pickingEvent_->setPickingEvent(false, mouseX_, mouseY_, Eigen::Vector3d::Zero(),
                                       Eigen::Vector3d::Zero(), Eigen::Vector3d::Zero(), nullptr);
}
std::string GraphicsView::clipboardText() const
{
    return driver_ ? driver_->clipboardText() : std::string{};
}
void GraphicsView::setClipboardText(const std::string &t)
{
    if (driver_)
        driver_->setClipboardText(t);
}
bool GraphicsView::hasTextInputEvents() const
{
    return driver_ && driver_->hasTextInputEvents();
}
void GraphicsView::setTextInputRect(int x, int y, int w, int h)
{
    if (driver_)
        driver_->setTextInputRect(x, y, w, h);
}
void GraphicsView::textInput(const std::string &t)
{
    updateGui();
    if (auto p = focusedGraphicsItem_)
        p->textInputEvent(t);
}
void GraphicsView::textComposition(const std::string &t, int cursor)
{
    updateGui();
    if (auto p = focusedGraphicsItem_)
        p->textCompositionEvent(t, cursor);
}
void GraphicsView::mouseLeave()
{
    mouseX_ = mouseY_ = -1;
    if (hoveredGraphicsItem_) {
        hoveredGraphicsItem_->setHoverd(false);
        hoveredGraphicsItem_->hoverLeaveEvent(graphicsItemHoverEvent_.get());
        hoveredGraphicsItem_.reset();
    }
}
void GraphicsView::cancelInput()
{
    cancelGraphicsItemDrag();
    setKeyboardFocus(nullptr);
    guiPointerCaptured_ = false;
}
} // namespace ptgl
