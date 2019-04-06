#ifndef PTGL_CORE_GRAPHICSITEMEVENT_H_
#define PTGL_CORE_GRAPHICSITEMEVENT_H_

#include "Event.h"

namespace ptgl {

class GraphicsItemEvent {
public:
    GraphicsItemEvent();
    virtual ~GraphicsItemEvent();
};

class GraphicsItemKeyEvent : public GraphicsItemEvent, public KeyEvent
{
public:
    GraphicsItemKeyEvent(GraphicsView* view) : KeyEvent(view) {}
    virtual ~GraphicsItemKeyEvent() {}
};

class GraphicsItemHoverEvent : public GraphicsItemEvent, public Event
{
public:
    GraphicsItemHoverEvent(GraphicsView* view) : Event(view) {}
    virtual ~GraphicsItemHoverEvent() {}

    virtual void setHoverEnterEvent(int x, int y);
    virtual void setHoverMoveEvent(int x, int y);
    virtual void setHoverLeaveEvent(int x, int y);

    int x() const { return x_; }
    int y() const { return y_; }
    int dx() const { return dx_; }
    int dy() const { return dy_; }
    int hoverX() const { return hoverX_; }
    int hoverY() const { return hoverY_; }
    int moveX() const { return moveX_; }
    int moveY() const { return moveY_; }

    double time() const;
    double hoverEnterTime() const { return hoverEnterTime_; }
    double hoverMoveTime() const { return hoverMoveTime_; }

protected:
    int x_ = 0;
    int y_ = 0;

    int dx_ = 0;
    int dy_ = 0;

    int hoverX_ = 0;
    int hoverY_ = 0;

    int moveX_ = 0;
    int moveY_ = 0;

    double hoverEnterTime_ = 0.0;
    double hoverMoveTime_ = 0.0;
};

class GraphicsItemMouseEvent : public GraphicsItemEvent, public MouseEvent
{
public:
    GraphicsItemMouseEvent(GraphicsView* view) : MouseEvent(view) {}
    virtual ~GraphicsItemMouseEvent() {}

    void setPickingEvent(bool isPicked) {
        isPicked_ = isPicked;
        origin_.setZero();
        direction_.setZero();
        point_.setZero();
    }

    void setPickingEvent(bool isPicked,
            const Eigen::Vector3d& origin,
            const Eigen::Vector3d& direction,
            const Eigen::Vector3d& point) {
        isPicked_ = isPicked;
        origin_ = origin;
        direction_ = direction;
        point_ = point;
    }

    bool isPicked() const { return isPicked_; }
    const Eigen::Vector3d& origin() const { return origin_; }            // camera origin
    const Eigen::Vector3d& direction() const { return direction_; }      // camera -> hit point
    const Eigen::Vector3d& point() const { return point_; }              // hit point

protected:
    bool isPicked_ = false;
    Eigen::Vector3d origin_ = Eigen::Vector3d::Zero();
    Eigen::Vector3d direction_ = Eigen::Vector3d::Zero();
    Eigen::Vector3d point_ = Eigen::Vector3d::Zero();
};

class GraphicsItemWheelEvent : public GraphicsItemEvent, public WheelEvent
{
public:
    GraphicsItemWheelEvent(GraphicsView* view) : WheelEvent(view) {}
    virtual ~GraphicsItemWheelEvent() {}
};

class GraphicsItemSelectEvent : public GraphicsItemEvent, public Event
{
public:
    GraphicsItemSelectEvent(GraphicsView* view) : Event(view) {}
    virtual ~GraphicsItemSelectEvent() {}
};

} /* namespace ptgl */

#endif /* PTGL_CORE_GRAPHICSITEMEVENT_H_ */
