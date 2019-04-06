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

void AbstractButton::setCheckable(bool on)
{
    checkable_ = on;
}

void AbstractButton::setDown(bool down)
{
    down_ = down;
    checked_ = down;
}

void AbstractButton::setText(const std::string& text)
{
    text_ = text;
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

void AbstractButton::setOnClickedFunction(std::function<void (bool)> func)
{
    onClickedFunc_ = func;
}

void AbstractButton::setOnPressedFunction(std::function<void ()> func)
{
    onPressedFunc_ = func;
}

void AbstractButton::setOnReleasedFunction(std::function<void ()> func)
{
    onReleasedFunc_ = func;
}

void AbstractButton::setOnToggledFunction(std::function<void (bool)> func)
{
    onToggledFunc_ = func;
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
