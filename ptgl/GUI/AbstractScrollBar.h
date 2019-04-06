#ifndef PTGL_GUI_ABSTRACTSCROLLBAR_H_
#define PTGL_GUI_ABSTRACTSCROLLBAR_H_

#include "Widget.h"

namespace ptgl {
namespace gui {

class AbstractScrollBar;
using AbstractScrollBarPtr = std::shared_ptr<AbstractScrollBar>;

class AbstractScrollBar : public Widget {
public:
    AbstractScrollBar();
    virtual ~AbstractScrollBar();

    virtual void setScrolledAreaSize(int size) { scrolledAreaSize_ = size; }
    int scrolledAreaSize() const { return scrolledAreaSize_; }

    virtual void setScrollValue(int value);
    int scrollValue() const { return scrollValue_; }

    virtual void setScrollDeltaValue(int delta);
    int scrollDeltaValue() const { return scrollDeltaValue_; }

    virtual int scrollAreaSize() const;

    std::pair<int, int> scrollArea() const {
        return std::pair<int, int>{scrollValue(), scrollValue() + scrollAreaSize()};
    }

protected:
    int scrolledAreaSize_;
    int scrollValue_;
    int scrollDeltaValue_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_ABSTRACTSCROLLBAR_H_ */
