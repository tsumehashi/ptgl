#ifndef PTGL_CORE_EVENT_H_
#define PTGL_CORE_EVENT_H_

#include <array>
#include <vector>
#include <string>
#include <Eigen/Core>
#include "Common.h"
#include "GraphicsItem.h"

namespace ptgl {

class GraphicsView;

class Event {
public:
    Event(GraphicsView* view);
    virtual ~Event();

    void setAccepted(bool accept) { accepted_ = accept; }
    bool isAccepted() const { return accepted_; }

protected:
    GraphicsView* view() { return view_; }
    const GraphicsView* view() const { return view_; }

    GraphicsView* view_ = nullptr;
    bool accepted_ = false;
};

class KeyEvent : public Event
{
public:

    enum class KeyAction {
        KeyPress,
        KeyRelease,
        KeyRepeat,
    };

    KeyEvent(GraphicsView* view);
    virtual ~KeyEvent();

    virtual void setKeyPressEvent(Key key, KeyAction keyAction = KeyAction::KeyPress, int modifierKey = ModifierKey_None);

    Key key() const { return key_; }
    KeyAction keyAction() const { return keyAction_; }
    int modifierKey() const { return modifierKey_; }

protected:
    Key key_ = Key::Key_Unknown;
    KeyAction keyAction_ = KeyAction::KeyPress;
    int modifierKey_ = ModifierKey_None;
};

class MouseEvent : public Event
{
public:

    enum class MouseButton {
        NoButton,
        LeftButton,
        RightButton,
        MiddleButton,
    };

    MouseEvent(GraphicsView* view);
    virtual ~MouseEvent();

    virtual void setPressEvent(int x, int y, MouseButton button, int modifierKey = ModifierKey_None);
    virtual void setMoveEvent(int x, int y, MouseButton button);
    virtual void setReleaseEvent(int x, int y, MouseButton button, int modifierKey = ModifierKey_None);
    virtual void clearEventState();

    uint64_t frame() const;

    uint64_t pressFrame(MouseButton button) const { return prevPrevPressFrameList_[(int)button]; }
    uint64_t prevPressFrame(MouseButton button) const { return prevPressFrameList_[(int)button]; }
    uint64_t prevPrevPressFrame(MouseButton button) const { return prevPrevPressFrameList_[(int)button]; }

    double pressTime(MouseButton button) const { return prevPrevPressTimeList_[(int)button]; }
    double prevPressTime(MouseButton button) const { return prevPressTimeList_[(int)button]; }
    double prevPrevPressTime(MouseButton button) const { return prevPrevPressTimeList_[(int)button]; }

    MouseButton button() const { return button_; }
    MouseButton pressedButton() const { return pressedButton_; }
    int x() const { return x_; }
    int y() const { return y_; }
    int dx() const { return dx_; }
    int dy() const { return dy_; }
    int pressX() const { return pressX_; }
    int pressY() const { return pressY_; }
    int moveX() const { return moveX_; }
    int moveY() const { return moveY_; }
    int modifierKey() const { return modifierKey_; }
    bool doubleClicked(MouseButton button) const;
    bool doubleClicked(MouseButton button, double deltaTime) const;
    bool doubleClicked(MouseButton button, uint64_t deltaFrame) const;

protected:
    int x_ = 0;
    int y_ = 0;

    int dx_ = 0;
    int dy_ = 0;

    int pressX_ = 0;
    int pressY_ = 0;

    int moveX_ = 0;
    int moveY_ = 0;

    MouseButton button_ = MouseButton::NoButton;
    MouseButton pressedButton_ = MouseButton::NoButton;
    int modifierKey_ = ModifierKey_None;

    std::array<uint64_t, 4> prevPrevPressFrameList_;
    std::array<uint64_t, 4> prevPressFrameList_;
    std::array<uint64_t, 4> pressFrameList_;

    std::array<double, 4> prevPrevPressTimeList_;
    std::array<double, 4> prevPressTimeList_;
    std::array<double, 4> pressTimeList_;
};

class WheelEvent : public Event
{
public:
    enum Orientation {
        Horizontal,
        Vertical,
    };

    WheelEvent(GraphicsView* view);
    virtual ~WheelEvent();

    virtual void setWheelEvent(int x, int y, int delta, Orientation orientation = Vertical);

    Orientation orientation() const { return orientation_; }
    int delta() const { return delta_; }
    int x() const { return x_; }
    int y() const { return y_; }

protected:
    int x_ = 0;
    int y_ = 0;
    int delta_ = 0;
    Orientation orientation_ = Vertical;
};

class PickingEvent : public Event
{
public:
    PickingEvent(GraphicsView* view);
    virtual ~PickingEvent();

    virtual void setPickingEvent(bool isPicked, int x, int y);

    virtual void setPickingEvent(bool isPicked,
            int x, int y,
            const Eigen::Vector3d& origin,
            const Eigen::Vector3d& direction,
            const Eigen::Vector3d& point,
            GraphicsItemPtr pickedGraphicsItem);

    int x() const { return x_; }    // mouse x
    int y() const { return y_; }    // mouse y

    bool isPicked() const { return isPicked_; }
    const Eigen::Vector3d& origin() const { return origin_; }                     // camera origin
    const Eigen::Vector3d& direction() const { return direction_; }               // camera -> hit point
    const Eigen::Vector3d& point() const { return point_; }                       // hit point
    GraphicsItemPtr pickedGraphicsItem() const { return pickedGraphicsItem_; }    // if (pickedGraphicsItem_ == null && isPicked == true) GraphicsView rendered is picked

protected:
    bool isPicked_ = false;
    int x_ = 0;    // mouse x
    int y_ = 0;    // mouse y

    Eigen::Vector3d origin_ = Eigen::Vector3d::Zero();
    Eigen::Vector3d direction_ = Eigen::Vector3d::Zero();
    Eigen::Vector3d point_ = Eigen::Vector3d::Zero();

    GraphicsItemPtr pickedGraphicsItem_;
};

class DropEvent : public Event
{
public:
    DropEvent(GraphicsView* view);
    virtual ~DropEvent();

    virtual void setDropEvent(const std::vector<std::string>& dropPaths) {
        dropPaths_ = dropPaths;
    }

    const std::vector<std::string>& dropPaths() const {
        return dropPaths_;
    }

protected:
    std::vector<std::string> dropPaths_;
};

} /* namespace ptgl */

#endif /* PTGL_CORE_EVENT_H_ */
