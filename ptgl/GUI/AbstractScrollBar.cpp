#include "AbstractScrollBar.h"

namespace ptgl {
namespace gui {

AbstractScrollBar::AbstractScrollBar() {
    scrolledAreaSize_ = 1;
    scrollValue_ = 0;
    scrollDeltaValue_ = 10;
}

AbstractScrollBar::~AbstractScrollBar() {

}

AbstractScrollBar& AbstractScrollBar::setScrollValue(int value)
{
    scrollValue_ = value;
    // clamp
    if (scrollValue_ < 0) {
        scrollValue_ = 0;
    } else if (scrollValue_ > (scrolledAreaSize_ - scrollAreaSize())) {
        scrollValue_ = scrolledAreaSize_ - scrollAreaSize();
    }
    return *this;
}

AbstractScrollBar& AbstractScrollBar::setScrollDeltaValue(int delta)
{
    scrollDeltaValue_ = delta;
    return *this;
}

int AbstractScrollBar::scrollAreaSize() const
{
    return width();
}

}
} /* namespace ptgl */
