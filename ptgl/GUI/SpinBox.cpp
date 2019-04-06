#include "SpinBox.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Core/Renderer2D.h"
#include "ptgl/Core/Font.h"

namespace ptgl {
namespace gui {

SpinBox::SpinBox()
{
    init();
}

SpinBox::SpinBox(const std::string& text)
    : AbstractSlider(text)
{
    init();
}

SpinBox::~SpinBox() {

}

void SpinBox::init()
{
    spinBoxSize_ = 7;
    spinBoxSpacing_ = 2;

    color_ = {{1.0,1.0,1.0,0.9}};

    spinUpButton_ = std::make_shared<PushButton>();
    spinDownButton_ = std::make_shared<PushButton>();

    spinUpButton_->setOnClickedFunction([&](bool on){ this->onSpinUpButtonClicled(on); });
    spinDownButton_->setOnClickedFunction([&](bool on){ this->onSpinDownButtonClicled(on); });

    spinUpButton_->setColor(0.6,0.6,0.6);
    spinDownButton_->setColor(0.6,0.6,0.6);

    // set tool tip
    spinUpButton_->setToolTipText("Range[" + std::to_string(this->minimum()) + ", " + std::to_string(this->maximum()) + "]");
    spinUpButton_->setEnableToolTip(true);
    spinUpButton_->setToolTipSize(18);
    spinDownButton_->setToolTipText("Range[" + std::to_string(this->minimum()) + ", " + std::to_string(this->maximum()) + "]");
    spinDownButton_->setEnableToolTip(true);
    spinDownButton_->setToolTipSize(18);

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

    setSize(40, 20);
}

void SpinBox::setSize(int width, int height)
{
    Graphics2DItem::setSize(width, height);

    spinTextSize_ = height * 0.9;
    spinTextWidth_ = ptgl::Font::defaultFontTextWidth(spinTextSize_, 5);

    spinUpButton_->setSize(spinBoxSize_, spinBoxSize_);
    spinDownButton_->setSize(spinBoxSize_, spinBoxSize_);

    textEdit_->setSize(width - spinBoxSize_, height);
    textEdit_->setLocalPos(0, 0);
}

void SpinBox::setRange(int min, int max)
{
    AbstractSlider::setRange(min, max);
    spinUpButton_->setToolTipText("Range[" + std::to_string(this->minimum()) + ", " + std::to_string(this->maximum()) + "]");
    spinDownButton_->setToolTipText("Range" + std::to_string(this->minimum()) + ", " + std::to_string(this->maximum()) + "]");
}

void SpinBox::setValue(int value, bool callCallback)
{
    AbstractSlider::setValue(value, callCallback);
    textEdit_->setText(std::to_string(this->value()), false);
}

void SpinBox::onSpinUpButtonClicled(bool)
{
    AbstractSlider::setValue(this->value()+1);
    textEdit_->setText(std::to_string(this->value()), false);
}

void SpinBox::onSpinDownButtonClicled(bool)
{
    AbstractSlider::setValue(this->value()-1);
    textEdit_->setText(std::to_string(this->value()), false);
}

void SpinBox::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    // [Value][SpinBox]

    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);
    r->setFillColor(color_);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);

    int localX = 0;

    // [Value]
#if 0
    int value = this->value();
    std::string text = std::to_string(value);
    int tx = x + localX + 2*spinBoxSpacing_;
    int ty = y + this->height() - (this->height() - spinTextSize_)/2;
    r->setTextSize(spinTextSize_);
    r->setTextColor(0.2,0.2,0.2);
    r->drawText(tx, ty, text);
#endif

    // [SpinBox]
    int sph = spinBoxSize_*2 + spinBoxSpacing_;
    int sux = w - spinBoxSize_;
    int suy = (h - sph)/2;
    int sdy = (h - sph)/2+sph-spinBoxSize_;
    spinUpButton_->setLocalPos(sux, suy);
    spinDownButton_->setLocalPos(sux, sdy);
    localX += spinBoxSize_;
}

void SpinBox::renderPicking2DScene(ptgl::Renderer2D* r)
{
    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    // draw background
    // [Value][SpinBox]
    r->setStrokeWeight(1);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);
}

void SpinBox::mousePressEvent(ptgl::GraphicsItemMouseEvent* e)
{
    (void)e;
}

}
} /* namespace ptgl */
