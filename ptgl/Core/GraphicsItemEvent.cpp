#include "GraphicsItemEvent.h"
#include "GraphicsView.h"

namespace ptgl {

GraphicsItemEvent::GraphicsItemEvent() {

}

GraphicsItemEvent::~GraphicsItemEvent() {

}

// GraphicsItemHoverEvent
void GraphicsItemHoverEvent::setHoverEnterEvent(int x, int y)
{
    x_ = x;
    y_ = y;
    dx_ = 0;
    dy_ = 0;
    hoverX_ = x;
    hoverY_ = y;
    moveX_ = 0;
    moveY_ = 0;

    hoverEnterTime_ = view()->currentTime();
    hoverMoveTime_ = hoverEnterTime_;
}

void GraphicsItemHoverEvent::setHoverMoveEvent(int x, int y)
{
    dx_ = x - x_;
    dy_ = y - y_;
    x_ = x;
    y_ = y;
    moveX_ = x - hoverX_;
    moveY_ = y - hoverY_;

    hoverMoveTime_ = view()->currentTime();
}

void GraphicsItemHoverEvent::setHoverLeaveEvent(int x, int y)
{
    x_ = x;
    y_ = y;
    dx_ = 0;
    dy_ = 0;
}

double GraphicsItemHoverEvent::time() const
{
    return view()->currentTime();
}

} /* namespace ptgl */
