#include "SpinSlider.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Core/GraphicsItemEvent.h"
#include "ptgl/Core/Renderer2D.h"
#include "ptgl/Core/Font.h"

namespace ptgl {
namespace gui {

SpinSlider::SpinSlider()
{
    init();
}

SpinSlider::SpinSlider(const std::string& text)
    : AbstractSlider(text)
{
    init();
}

SpinSlider::~SpinSlider()
{

}

void SpinSlider::init()
{
    showText_ = true;
    showTextLength_ = 14;
    sliderSpacing_ = 10;
    spinBoxSize_ = 7;
    spinBoxSpacing_ = 2;

    color_ = {{1.0,1.0,1.0,0.9}};

    slider_ = std::make_shared<Slider>();
    spinUpButton_ = std::make_shared<PushButton>();
    spinDownButton_ = std::make_shared<PushButton>();

    slider_->setOnValueChangedFunction([&](int value){ this->onSliderValueChanged(value); });
    spinUpButton_->setOnClickedFunction([&](bool on){ this->onSpinUpButtonClicled(on); });
    spinDownButton_->setOnClickedFunction([&](bool on){ this->onSpinDownButtonClicled(on); });

    spinUpButton_->setColor(0.6,0.6,0.6);
    spinDownButton_->setColor(0.6,0.6,0.6);

    this->addWidget(slider_);
    this->addWidget(spinUpButton_);
    this->addWidget(spinDownButton_);

    textEdit_ = std::make_shared<TextEditWidget>();
    textEdit_->setFilterRegexText(TextEditFilter::Regex::IntegerNumber);
    textEdit_->setOnTextChangedFunction([&](const std::string& text, const std::string& prevText){
        (void)prevText;
        // check text
        int val = 0;
        bool result = false;
        try {
            val = std::stoi(text);
            result = true;
        } catch (...) {
            result = false;
        }
        if (result) {
            setValue(val, true);
        } else {
            textEdit_->setText(std::to_string(this->value()), false);    // restore
        }
    });
    this->addWidget(textEdit_);
    textEdit_->setText(std::to_string(this->value()), false);
    textEdit_->setToolTipText("Range[" + std::to_string(this->minimum()) + ", " + std::to_string(this->maximum()) + "]");
    textEdit_->setEnableToolTip(true);
    textEdit_->setToolTipSize(18);

    // enable wheel event
    setEnabledWheelEvent(true);

    setSize(200, 20);
}

void SpinSlider::setText(const std::string& text)
{
    AbstractSlider::setText(text);

    // resize
    setSize(width(), height());
}

void SpinSlider::setShowText(bool on)
{
    showText_ = on;
}

void SpinSlider::setShowTextLength(int length)
{
    showTextLength_ = length;
}

void SpinSlider::setSize(int width, int height)
{
    Graphics2DItem::setSize(width, height);

    spinTextSize_ = height * 0.9;
    spinTextWidth_ = ptgl::Font::defaultFontTextWidth(spinTextSize_, 5);

    int titleWidth = 0;
    if (showText_) {
        titleTextSize_ = height * 0.9;
        int numText = showTextLength_ > 0 ? showTextLength_ : (int)this->text().size();
        titleTextWidth_ = ptgl::Font::defaultFontTextWidth(titleTextSize_, numText);
        titleWidth = titleTextWidth_;
    }

    sliderLength_ = width - titleWidth - spinTextWidth_ - 2*sliderSpacing_ - spinBoxSize_ - 2*spinBoxSpacing_;

    // set minimum
    if (sliderLength_ < 10) sliderLength_ = 10;

    slider_->setSliderLength(sliderLength_);

    spinUpButton_->setSize(spinBoxSize_, spinBoxSize_);
    spinDownButton_->setSize(spinBoxSize_, spinBoxSize_);
}

void SpinSlider::setRange(int min, int max)
{
    AbstractSlider::setRange(min, max);
    slider_->setRange(min, max);
    textEdit_->setToolTipText("Range[" + std::to_string(this->minimum()) + ", " + std::to_string(this->maximum()) + "]");
}

void SpinSlider::setValue(int value, bool callCallback)
{
    AbstractSlider::setValue(value, callCallback);
    slider_->setValue(value);
    textEdit_->setText(std::to_string(slider_->value()), false);
}

void SpinSlider::onSliderValueChanged(int value)
{
    AbstractSlider::setValue(value);
    textEdit_->setText(std::to_string(slider_->value()), false);
}

void SpinSlider::onSpinUpButtonClicled(bool)
{
    slider_->setValue(slider_->value()+1);
    textEdit_->setText(std::to_string(slider_->value()), false);
}

void SpinSlider::onSpinDownButtonClicled(bool)
{
    slider_->setValue(slider_->value()-1);
    textEdit_->setText(std::to_string(slider_->value()), false);
}

void SpinSlider::onFocus()
{
    if (onFocusedFunc_) {
        onFocusedFunc_();
    }
}

void SpinSlider::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    // [Title][Slider][SpinBox][Value]

    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);
    r->setFillColor(color_);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);

    int localX = 0;

    // [Title]
    if (showText_) {
        int textSpacing = 4;
        int tx = x + textSpacing;
        int ty = y + this->height() - (this->height() - titleTextSize_)/2;

        r->setTextSize(titleTextSize_);
        if ((int)text().size() > showTextLength_) {
            r->setTextColor(0,0,0,0.2);
            r->drawText(tx, ty, text());

            // overwrite
            std::string str = text().substr(0, showTextLength_);
            r->setTextColor(0,0,0);
            r->drawText(tx, ty, str);
        } else {
            r->setTextColor(0,0,0);
            r->drawText(tx, ty, text());
        }

        localX = textSpacing + titleTextWidth_;
    }

    // [Slider]
    int sx = localX + sliderSpacing_;
    int sy = h/2;
    slider_->setLocalPos(sx, sy);
    localX += sliderSpacing_ + sliderLength_;

    // [SpinBox]
    int sph = spinBoxSize_*2 + spinBoxSpacing_;
    int sux = localX + sliderSpacing_;
    int suy = (h - sph)/2;
    int sdy = (h - sph)/2+sph-spinBoxSize_;
    spinUpButton_->setLocalPos(sux, suy);
    spinDownButton_->setLocalPos(sux, sdy);
    localX += sliderSpacing_ + spinBoxSize_;

    // [Value]
    int textEditWidth = w - localX;
    textEdit_->setSize(textEditWidth, h);
    textEdit_->setLocalPos(localX, 0);
#if 0
    int value = slider_->value();
    std::string text = std::to_string(value);
    int tx = x + localX + 2*spinBoxSpacing_;
    int ty = y + this->height() - (this->height() - spinTextSize_)/2;
    r->setTextSize(spinTextSize_);
    r->setTextColor(0.2,0.2,0.2);
    r->drawText(tx, ty, text);
#endif
}

void SpinSlider::renderPicking2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    // draw background
    // [Title][Slider][SpinBox][Value]
    r->setStrokeWeight(1);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);
}

void SpinSlider::mousePressEvent(ptgl::GraphicsItemMouseEvent* e)
{
    (void)e;
    if (checkIsDoubleClick(e)) {
        // double clicked
        if (onDoubleClickedFunc_) {
            onDoubleClickedFunc_();
        }
    } else {
        onFocus();
    }
}

void SpinSlider::wheelEvent(ptgl::GraphicsItemWheelEvent* e)
{
    if (!e->isAccepted() && isHoverd()) {
        int delta = 0.01 * (this->maximum() - this->minimum());
        if (delta < 1) delta = 1;
        if (e->delta() < 0) delta = -delta;
        slider_->setValue(slider_->value() + delta, true);
        textEdit_->setText(std::to_string(slider_->value()), false);

        e->setAccepted(true);
    }
}

bool SpinSlider::checkIsDoubleClick(ptgl::GraphicsItemMouseEvent* e) const
{
    return e->doubleClicked(MouseEvent::MouseButton::LeftButton, doubleClickTime_);
}

}
} /* namespace ptgl */
