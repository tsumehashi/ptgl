#include "Widget.h"
#include "ptgl/Core/GraphicsItemEvent.h"
#include "ptgl/Core/GraphicsView.h"

namespace ptgl {
namespace gui {

Widget::Widget()
{
    tootTipTextSize_ = 18;
    toolTipTextColor_ = {{1.0, 1.0, 1.0, 1.0}};
    toolTipBackgroundColor_ = {{0.1, 0.1, 0.1, 1.0}};
    toolTipWaitShowTime_ = 0.5;
    hoverNotMovedTime_ = 0.0;
}

Widget::~Widget()
{

}

void Widget::setTheme(const Theme &t)
{
    t.validate();
    theme_ = std::make_shared<const Theme>(t);
}
const Theme &Widget::theme() const
{
    if (theme_)
        return *theme_;
    if (auto p = dynamic_cast<const Widget *>(parentItem()))
        return p->theme();
    static const Theme defaults;
    return defaults;
}
Rect Widget::clipRect() const
{
    Rect result{0, 0, graphicsWindow() ? viewWidth() : 1000000, graphicsWindow() ? viewHeight() : 1000000};
    if (popup_)
        return result;
    for (auto p = parentItem(); p; p = p->parentItem())
        if (auto w = dynamic_cast<const Widget *>(p)) {
            if (w->clipChildren_)
                result = result.intersect(w->rect());
            if (w->popup_)
                break;
        }
    return result;
}
bool Widget::isPopupLayer() const
{
    for (auto p = this; p; p = dynamic_cast<const Widget *>(p->parentItem()))
        if (p->popup_)
            return true;
    return false;
}
bool Widget::hitTest(int px, int py) const
{
    return isVisible() && isEnabled() && isPickable() && rect().intersect(clipRect()).contains(px, py);
}
void Widget::setPreferredSize(int w, int h)
{
    if (w < 0 || h < 0)
        throw std::invalid_argument("Negative GUI size");
    preferred_ = {w, h};
}
void Widget::setMinimumSize(int w, int h)
{
    if (w < 0 || h < 0)
        throw std::invalid_argument("Negative GUI minimum");
    minimum_ = {w, h};
}
std::array<int, 2> Widget::sizeHint() const
{
    return {theme().pixels(std::max(preferred_[0], minimum_[0])),
            theme().pixels(std::max(preferred_[1], minimum_[1]))};
}
std::array<int, 2> Widget::minimumSize() const
{
    return {theme().pixels(minimum_[0]), theme().pixels(minimum_[1])};
}
void Widget::drawFrame(Renderer2D *r, bool active) const
{
    const auto &t = theme();
    r->setRectMode(Renderer2D::Mode::Corner);
    r->setStrokeWeight(isPicked() ? 2 : 1);
    r->setStrokeColor(isPicked() ? t.accent : t.border);
    r->setFillColor(active ? t.checked : t.field);
    r->drawRect(x(), y(), width(), height());
}
void Widget::drawCaption(Renderer2D *r, const std::string &text, int inset) const
{
    r->setTextSize(theme().pixels(theme().fontSize));
    r->setTextColor(isEnabled() ? theme().text : theme().muted);
    r->drawText(x() + theme().pixels(inset), y() + (height() + r->textHeight()) / 2, text);
}
void Widget::cancelInteraction()
{
    GraphicsItem::cancelInteraction();
    drawToolTip_ = false;
}

void Widget::addWidget(WidgetPtr widget)
{
    addChild(widget);
}

void Widget::removeWidget(WidgetPtr widget)
{
    removeChild(widget);
}

void Widget::renderTextScene(ptgl::TextRenderer* r)
{
    if (isEnabledToolTip() && checkDrawToolTip()) {
        r->setTextColor(toolTipTextColor_);
        r->setTextSize(toolTipTextSize());

        int offset = 15;
        int tx = toolTipLockX() + offset;
        int ty = toolTipLockY() + toolTipTextSize() + offset;
        r->drawText(tx, ty, toolTipText(), toolTipBackgroundColor_);
    }
}

void Widget::hoverEnterEvent(ptgl::GraphicsItemHoverEvent* e)
{
    drawToolTip_ = false;
    hoverNotMovedTime_ = e->time();
    hoverLockX_ = e->x();
    hoverLockY_ = e->y();
}

void Widget::hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* e)
{
    (void)e;
    drawToolTip_ = false;
}

void Widget::hoverMoveEvent(ptgl::GraphicsItemHoverEvent* e)
{
    hoverNotMovedTime_ = e->time();

    if (this->graphicsWindow()) {
        if ((this->graphicsWindow()->currentTime() - hoverNotMovedTime_) > toolTipWaitShowTime_) {
            drawToolTip_ = true;
        }
    }

    if (!drawToolTip_) {
        hoverLockX_ = e->x();
        hoverLockY_ = e->y();
    }
}

bool Widget::checkDrawToolTip()
{
    if (!isHoverd()) {
        return false;
    }

    if (this->graphicsWindow()) {
        if ((this->graphicsWindow()->currentTime() - hoverNotMovedTime_) > toolTipWaitShowTime_) {
            drawToolTip_ = true;
        }
    }

    return drawToolTip_;
}

}
} /* namespace ptgl */
