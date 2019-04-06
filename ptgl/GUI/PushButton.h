#ifndef PTGL_GUI_PUSHBUTTON_H_
#define PTGL_GUI_PUSHBUTTON_H_

#include <array>
#include "AbstractButton.h"

namespace ptgl {
namespace gui {

class Renderer2D;

class PushButton;
typedef std::shared_ptr<PushButton> PushButtonPtr;

class PushButton : public AbstractButton {
public:
    PushButton();
    PushButton(const std::string& text);
    virtual ~PushButton();

    void setColor(const std::array<double, 4>& color);
    void setColor(double r, double g, double b, double a = 1.0);
    const std::array<double, 4>& color() const { return color_; }

    void setCheckedColor(const std::array<double, 4>& color);
    void setCheckedColor(double r, double g, double b, double a = 1.0);
    const std::array<double, 4>& checkedColor() const { return checkedColor_; }

    virtual void setText(const std::string& text) override;

protected:
    virtual void render2DScene(ptgl::Renderer2D* r) override;

    std::array<double, 4> color_;
    std::array<double, 4> checkedColor_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_PUSHBUTTON_H_ */
