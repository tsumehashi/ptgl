#ifndef PTGL_GUI_ABSTRACTBUTTON_H_
#define PTGL_GUI_ABSTRACTBUTTON_H_

#include <functional>
#include "Widget.h"

namespace ptgl {
namespace gui {

class AbstractButton;
typedef std::shared_ptr<AbstractButton> AbstractButtonPtr;

class AbstractButton : public Widget {
public:
    AbstractButton();
    AbstractButton(const std::string& text);
    virtual ~AbstractButton();

    bool isCheckable() const { return checkable_; }
    bool isChecked() const { return checked_; }
    bool isDown() const { return down_; }

    virtual AbstractButton& setCheckable(bool on);
    virtual AbstractButton& setDown(bool down);
    virtual AbstractButton& setText(const std::string& text);

    const std::string& text() const { return text_; }

    virtual void click();
    virtual void setChecked(bool checked);
    virtual void toggle();

    AbstractButton& setOnClickedFunction(std::function<void (bool)> func);
    AbstractButton& setOnPressedFunction(std::function<void ()> func);
    AbstractButton& setOnReleasedFunction(std::function<void ()> func);
    AbstractButton& setOnToggledFunction(std::function<void (bool)> func);

protected:

    // Mouse event
    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void mouseReleaseEvent(ptgl::GraphicsItemMouseEvent* e) override;

    bool checkable_ = false;
    bool checked_ = false;
    bool down_ = false;
    std::string text_;

    std::function<void (bool)> onClickedFunc_;
    std::function<void ()> onPressedFunc_;
    std::function<void ()> onReleasedFunc_;
    std::function<void (bool)> onToggledFunc_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_ABSTRACTBUTTON_H_ */
