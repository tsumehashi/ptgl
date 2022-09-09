#include "AbstractButton.h"

namespace ptgl {
namespace gui {

AbstractButton::AbstractButton()
{

}

AbstractButton::AbstractButton(const std::string& text)
    : text_(text)
{

}

AbstractButton::~AbstractButton()
{

}

AbstractButton& AbstractButton::setCheckable(bool on)
{
    checkable_ = on;
    return *this;
}

AbstractButton& AbstractButton::setDown(bool down)
{
    down_ = down;
    checked_ = down;
    return *this;
}

AbstractButton& AbstractButton::setText(const std::string& text)
{
    text_ = text;
    return *this;
}

void AbstractButton::click()
{

}

void AbstractButton::setChecked(bool checked)
{
    checked_ = checked;
    down_ = checked;
}

void AbstractButton::toggle()
{

}

AbstractButton& AbstractButton::setOnClickedFunction(std::function<void (bool)> func)
{
    onClickedFunc_ = func;
    return *this;
}

AbstractButton& AbstractButton::setOnPressedFunction(std::function<void ()> func)
{
    onPressedFunc_ = func;
    return *this;
}

AbstractButton& AbstractButton::setOnReleasedFunction(std::function<void ()> func)
{
    onReleasedFunc_ = func;
    return *this;
}

AbstractButton& AbstractButton::setOnToggledFunction(std::function<void (bool)> func)
{
    onToggledFunc_ = func;
    return *this;
}

void AbstractButton::mousePressEvent(ptgl::GraphicsItemMouseEvent* /*e*/)
{
    if (isCheckable()) {

    } else {
        checked_ = true;
        setDown(isChecked());
    }

    if (onPressedFunc_) {
        onPressedFunc_();
    }
}

void AbstractButton::mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* /*e*/)
{
    if (isCheckable()) {
        checked_ = !checked_;
        setDown(isChecked());

        if (onToggledFunc_) {
            onToggledFunc_(isChecked());
        }
    } else {
        checked_ = false;
        setDown(isChecked());
    }

    if (onClickedFunc_) {
        onClickedFunc_(isChecked());
    }

    if (onReleasedFunc_) {
        onReleasedFunc_();
    }
}

}
} /* namespace ptgl */
