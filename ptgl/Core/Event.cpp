#include "Event.h"
#include "GraphicsView.h"

namespace ptgl {

// Event
Event::Event(GraphicsView* view)
    : view_(view)
{

}

Event::~Event()
{

}

// KeyEvent
KeyEvent::KeyEvent(GraphicsView* view)
    : Event(view)
{

}

KeyEvent::~KeyEvent()
{

}

void KeyEvent::setKeyPressEvent(Key key, KeyAction keyAction, int modifierKey)
{
    key_ = key;
    keyAction_ = keyAction;
    modifierKey_ = modifierKey;
}

// MouseEvent
MouseEvent::MouseEvent(GraphicsView* view)
    : Event(view)
{
    prevPrevPressFrameList_.fill(view->currentFrame());
    prevPressFrameList_.fill(view->currentFrame());
    pressFrameList_.fill(view->currentFrame());

    prevPrevPressTimeList_.fill(view->currentTime());
    prevPressTimeList_.fill(view->currentTime());
    pressTimeList_.fill(view->currentTime());
}

MouseEvent::~MouseEvent()
{

}

uint64_t MouseEvent::frame() const
{
    return view_->currentFrame();
}

void MouseEvent::setPressEvent(int x, int y, MouseButton button, int modifierKey)
{
    x_ = x;
    y_ = y;
    dx_ = 0;
    dy_ = 0;
    pressX_ = x;
    pressY_ = y;
    moveX_ = 0;
    moveY_ = 0;
    button_ = button;
    pressedButton_ = button;
    modifierKey_ = modifierKey;

    // update press Frame
    prevPrevPressFrameList_[(int)button] = prevPressFrameList_[(int)button];
    prevPressFrameList_[(int)button] = pressFrameList_[(int)button];
    pressFrameList_[(int)button] = view()->currentFrame();

    // update press time
    prevPrevPressTimeList_[(int)button] = prevPressTimeList_[(int)button];
    prevPressTimeList_[(int)button] = pressTimeList_[(int)button];
    pressTimeList_[(int)button] = view()->currentTime();
}

void MouseEvent::setMoveEvent(int x, int y, MouseButton button)
{
    dx_ = x - x_;
    dy_ = y - y_;
    x_ = x;
    y_ = y;
    moveX_ = x - pressX_;
    moveY_ = y - pressY_;
    button_ = button;
    modifierKey_ = ModifierKey_None;    // always none
}

void MouseEvent::setReleaseEvent(int x, int y, MouseButton button, int modifierKey)
{
    x_ = x;
    y_ = y;
    dx_ = 0;
    dy_ = 0;
    button_ = button;
    pressedButton_ = MouseButton::NoButton;
    modifierKey_ = modifierKey;
}

void MouseEvent::clearEventState()
{
    for (int i = 0; i < 3; ++i) {
        prevPrevPressFrameList_[i] = prevPressFrameList_[i];
        prevPrevPressTimeList_[i] = prevPressTimeList_[i];
    }
}

bool MouseEvent::doubleClicked(MouseButton button) const
{
    const double defaultDoubleClickTime = 0.5;
    return doubleClicked(button, defaultDoubleClickTime);
}

bool MouseEvent::doubleClicked(MouseButton button, double deltaTime) const
{
    double df = pressTimeList_[(int)button] - prevPressTimeList_[(int)button];
    double pdf = prevPressTimeList_[(int)button] - prevPrevPressTimeList_[(int)button];
    return (df < deltaTime) && (pdf > deltaTime);
}

bool MouseEvent::doubleClicked(MouseButton button, uint64_t deltaFrame) const
{
    double df = pressFrameList_[(int)button] - prevPressFrameList_[(int)button];
    double pdf = prevPressFrameList_[(int)button] - prevPrevPressFrameList_[(int)button];
    return (df < deltaFrame) && (pdf > deltaFrame);
}

// WheelEvent
WheelEvent::WheelEvent(GraphicsView* view)
    : Event(view)
{

}

WheelEvent::~WheelEvent()
{

}

void WheelEvent::setWheelEvent(int x, int y, int delta, Orientation orientation)
{
    x_ = x, y_ = y, delta_ = delta, orientation_ = orientation;
}

// PickingEvent
PickingEvent::PickingEvent(GraphicsView* view)
    : Event(view)
{

}

PickingEvent::~PickingEvent() {

}

void PickingEvent::setPickingEvent(bool isPicked, int x, int y)
{
    isPicked_ = isPicked;
    x_ = x;
    y_ = y;
    origin_.setZero();
    direction_.setZero();
    point_.setZero();
    pickedGraphicsItem_ = nullptr;
}

void PickingEvent::setPickingEvent(bool isPicked,
        int x, int y,
        const Eigen::Vector3d& origin,
        const Eigen::Vector3d& direction,
        const Eigen::Vector3d& point,
        GraphicsItemPtr pickedGraphicsItem)
{
    isPicked_ = isPicked;
    x_ = x;
    y_ = y;
    origin_ = origin;
    direction_ = direction;
    point_ = point;
    pickedGraphicsItem_ = pickedGraphicsItem;
}

// DropEvent
DropEvent::DropEvent(GraphicsView* view)
    : Event(view)
{

}

DropEvent::~DropEvent()
{

}

} /* namespace ptgl */
