#ifndef PTGL_GUI_ABSTRACTSLIDER_H_
#define PTGL_GUI_ABSTRACTSLIDER_H_

#include <functional>
#include "Widget.h"

namespace ptgl {
namespace gui {

class AbstractSlider;
typedef std::shared_ptr<AbstractSlider> AbstractSliderPtr;

class AbstractSlider : public Widget {
public:
    AbstractSlider();
    AbstractSlider(const std::string& text);
    virtual ~AbstractSlider();

    virtual AbstractSlider& setText(const std::string& text);

    const std::string& text() const { return text_; }

    AbstractSlider& setMaximum(int max);
    AbstractSlider& setMinimum(int min);
    virtual AbstractSlider& setRange(int min, int max);
    int maximum() const { return maximum_; }
    int minimum() const { return minimum_; }

    virtual AbstractSlider& setValue(int value, bool callCallback = true);
    int value() const { return value_; }

    AbstractSlider& setOnValueChangedFunction(std::function<void (int)> func);

protected:
    std::string text_;

    int maximum_ = 100;
    int minimum_ = 0;
    int value_ = 0;

    std::function<void (int)> onValueChangedFunc_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_ABSTRACTSLIDER_H_ */
