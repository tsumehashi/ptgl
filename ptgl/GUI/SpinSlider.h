#ifndef PTGL_GUI_SPINSLIDER_H_
#define PTGL_GUI_SPINSLIDER_H_

#include <functional>
#include "AbstractSlider.h"
#include "PushButton.h"
#include "Slider.h"
#include "TextEditWidget.h"

namespace ptgl {
namespace gui {

class Renderer2D;

class SpinSlider;
typedef std::shared_ptr<SpinSlider> SpinSliderPtr;

class SpinSlider : public AbstractSlider {
public:
    SpinSlider();
    SpinSlider(const std::string& text);
    virtual ~SpinSlider();

    virtual void setText(const std::string& text) override;

    void setShowText(bool on);
    void setShowTextLength(int length);

    virtual void setSize(int width, int height) override;

    virtual void setRange(int min, int max) override;
    virtual void setValue(int value, bool callCallback = true) override;

    // color
    void setColor(const std::array<double, 4>& color) { color_ = color; }
    const std::array<double, 4>& color() const { return color_; }

    void setEnableEditText(bool enable) { textEdit_->setEnableEditText(enable); }
    bool enableEditText() const { return textEdit_->enableEditText(); }

    // double click
    void setDoubleClickTime(double time) { doubleClickTime_ = time; }
    double doubleClickTime() const { return doubleClickTime_; }

    void setOnFocusedFunction(std::function<void ()> func) { onFocusedFunc_ = func; }
    void setOnDoubleClickedFunction(std::function<void ()> func) { onDoubleClickedFunc_ = func; }

protected:

    virtual void render2DScene(ptgl::Renderer2D* r) override;
    virtual void renderPicking2DScene(ptgl::Renderer2D* r) override;
    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;
    virtual void wheelEvent(ptgl::GraphicsItemWheelEvent* e) override;

    void init();
    void onSliderValueChanged(int value);
    void onSpinUpButtonClicled(bool);
    void onSpinDownButtonClicled(bool);
    void onFocus();
    bool checkIsDoubleClick(ptgl::GraphicsItemMouseEvent* e) const;

    SliderPtr slider_;
    PushButtonPtr spinUpButton_;
    PushButtonPtr spinDownButton_;
    TextEditWidgetPtr textEdit_;

    bool showText_;
    int showTextLength_;    //0:nolimit

    int titleTextSize_;
    int titleTextWidth_;

    int sliderLength_;
    int sliderSpacing_;
    int spinBoxSize_;
    int spinBoxSpacing_;
    int spinTextSize_;
    int spinTextWidth_;

    double doubleClickTime_ = 0.05;

    std::array<double, 4> color_;

    std::function<void ()> onFocusedFunc_;
    std::function<void ()> onDoubleClickedFunc_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_SPINSLIDER_H_ */
