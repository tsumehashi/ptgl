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

void AbstractSlider::setText(const std::string& text)
{
    text_ = text;
}

void AbstractSlider::setMaximum(int max)
{
    setRange(minimum(), max);
}

void AbstractSlider::setMinimum(int min)
{
    setRange(min, maximum());
}

void AbstractSlider::setRange(int min, int max)
{
    maximum_ = max;
    minimum_ = min;
    if (value_ < minimum_) {
        setValue(minimum_);
    }

    if (value_ > maximum_) {
        setValue(maximum_);
    }
}

void AbstractSlider::setValue(int value, bool callCallback)
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
}

void AbstractSlider::setOnValueChangedFunction(std::function<void (int)> func)
{
    onValueChangedFunc_ = func;
}

}
} /* namespace ptgl */
