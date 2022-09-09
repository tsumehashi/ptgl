#include "AbstractSlider.h"

namespace ptgl {
namespace gui {

AbstractSlider::AbstractSlider()
{

}

AbstractSlider::AbstractSlider(const std::string& text)
    : text_(text)
{

}

AbstractSlider::~AbstractSlider()
{

}

AbstractSlider& AbstractSlider::setText(const std::string& text)
{
    text_ = text;
    return *this;
}

AbstractSlider& AbstractSlider::setMaximum(int max)
{
    return setRange(minimum(), max);
}

AbstractSlider& AbstractSlider::setMinimum(int min)
{
    return setRange(min, maximum());
}

AbstractSlider& AbstractSlider::setRange(int min, int max)
{
    maximum_ = max;
    minimum_ = min;
    if (value_ < minimum_) {
        setValue(minimum_);
    }

    if (value_ > maximum_) {
        setValue(maximum_);
    }
    return *this;
}

AbstractSlider& AbstractSlider::setValue(int value, bool callCallback)
{
    int temp = value;
    if (temp < minimum_) temp = minimum_;
    if (temp > maximum_) temp = maximum_;

    if (value_ != temp) {
        value_ = temp;

        if (callCallback && onValueChangedFunc_) {
            onValueChangedFunc_(value);
        }
    }
    return *this;
}

AbstractSlider& AbstractSlider::setOnValueChangedFunction(std::function<void (int)> func)
{
    onValueChangedFunc_ = func;
    return *this;
}

}
} /* namespace ptgl */
