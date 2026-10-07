#include "AbstractButton.h"
#include "ptgl/Core/GraphicsItemEvent.h"
namespace ptgl::gui
{
AbstractButton::AbstractButton()
{
    setFocusable(true);
}
AbstractButton::AbstractButton(const std::string &text) : AbstractButton()
{
    setText(text);
}
AbstractButton::~AbstractButton() = default;
AbstractButton &AbstractButton::setCheckable(bool on)
{
    checkable_ = on;
    if (!on)
        setChecked(false);
    return *this;
}
AbstractButton &AbstractButton::setDown(bool on)
{
    down_ = on;
    return *this;
}
AbstractButton &AbstractButton::setText(const std::string &text)
{
    text_ = text;
    return *this;
}
void AbstractButton::setChecked(bool on)
{
    if (checked_ == on)
        return;
    checked_ = on;
    auto f = onToggledFunc_;
    if (f)
        f(on);
}
void AbstractButton::toggle()
{
    if (checkable_)
        setChecked(!checked_);
}
void AbstractButton::click()
{
    if (!isEnabled())
        return;
    auto pressed = onPressedFunc_, released = onReleasedFunc_;
    auto clicked = onClickedFunc_;
    down_ = true;
    if (pressed)
        pressed();
    down_ = false;
    toggle();
    if (released)
        released();
    if (clicked)
        clicked(checked_);
}
AbstractButton &AbstractButton::setOnClickedFunction(std::function<void(bool)> f)
{
    onClickedFunc_ = std::move(f);
    return *this;
}
AbstractButton &AbstractButton::setOnPressedFunction(std::function<void()> f)
{
    onPressedFunc_ = std::move(f);
    return *this;
}
AbstractButton &AbstractButton::setOnReleasedFunction(std::function<void()> f)
{
    onReleasedFunc_ = std::move(f);
    return *this;
}
AbstractButton &AbstractButton::setOnToggledFunction(std::function<void(bool)> f)
{
    onToggledFunc_ = std::move(f);
    return *this;
}
void AbstractButton::mousePressEvent(GraphicsItemMouseEvent *e)
{
    if (e->button() != MouseEvent::MouseButton::LeftButton)
        return;
    armed_ = down_ = true;
    e->setAccepted(true);
    auto f = onPressedFunc_;
    if (f)
        f();
}
void AbstractButton::mouseMoveEvent(GraphicsItemMouseEvent *e)
{
    if (armed_)
        down_ = hitTest(e->x(), e->y());
}
void AbstractButton::mouseReleaseEvent(GraphicsItemMouseEvent *e)
{
    if (!armed_)
        return;
    bool activate = isEnabled() && hitTest(e->x(), e->y());
    armed_ = down_ = false;
    e->setAccepted(true);
    auto released = onReleasedFunc_;
    auto clicked = onClickedFunc_;
    if (activate)
        toggle();
    if (released)
        released();
    if (activate && clicked)
        clicked(checked_);
}
void AbstractButton::keyPressEvent(GraphicsItemKeyEvent *e)
{
    if (e->key() != Key::Key_Space && e->key() != Key::Key_Enter && e->key() != Key::Key_Return)
        return;
    e->setAccepted(true);
    if (e->keyAction() == KeyEvent::KeyAction::KeyPress)
        click();
}
void AbstractButton::cancelInteraction()
{
    Widget::cancelInteraction();
    armed_ = down_ = false;
}
} // namespace ptgl::gui
