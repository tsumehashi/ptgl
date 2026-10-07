#include "AbstractScrollBar.h"
#include <algorithm>

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
    scrollValue_ = std::clamp(value, 0, std::max(0, scrolledAreaSize_ - scrollAreaSize()));
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
