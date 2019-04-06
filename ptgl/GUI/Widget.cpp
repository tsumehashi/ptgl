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
