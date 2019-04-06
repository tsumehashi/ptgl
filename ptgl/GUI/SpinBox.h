#ifndef PTGL_GUI_SPINBOX_H_
#define PTGL_GUI_SPINBOX_H_

#include <functional>
#include "AbstractSlider.h"
#include "PushButton.h"
#include "TextEditWidget.h"

namespace ptgl {
namespace gui {

class SpinBox : public AbstractSlider {
public:
    SpinBox();
    SpinBox(const std::string& text);
    virtual ~SpinBox();

    virtual void setSize(int width, int height) override;

    virtual void setRange(int min, int max) override;
    virtual void setValue(int value, bool callCallback = true) override;

    // color
    void setColor(const std::array<double, 4>& color) { color_ = color; }
    const std::array<double, 4>& color() const { return color_; }

    void setEnableEditText(bool enable) { textEdit_->setEnableEditText(enable); }
    bool enableEditText() const { return textEdit_->enableEditText(); }

protected:
    virtual void render2DScene(ptgl::Renderer2D* r) override;
    virtual void renderPicking2DScene(ptgl::Renderer2D* r) override;
    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;

    void init();
    void onSpinUpButtonClicled(bool);
    void onSpinDownButtonClicled(bool);

    PushButtonPtr spinUpButton_;
    PushButtonPtr spinDownButton_;
    TextEditWidgetPtr textEdit_;

    int spinBoxSize_;
    int spinBoxSpacing_;
    int spinTextSize_;
    int spinTextWidth_;

    std::array<double, 4> color_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_SPINBOX_H_ */
