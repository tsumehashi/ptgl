#include "Slider.h"
#include "ptgl/Core/GraphicsItemEvent.h"
#include "ptgl/Core/Renderer2D.h"

namespace ptgl {
namespace gui {

Slider::Slider()
{
    init();
}

Slider::Slider(const std::string& text)
    : AbstractSlider(text)
{
    init();
}

Slider::~Slider()
{

}

void Slider::init()
{
    sliderHandle_ = std::make_shared<SliderHandle>(this);
    this->addWidget(sliderHandle_);
}

void Slider::setSliderLength(int sliderLength)
{
    sliderLength_ = sliderLength;
}

void Slider::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();
    int x = this->x();
    int y = this->y();
    int beginX = x;
    int endX = x + sliderLength();

    // draw base line
    r->setStrokeColor(0.2,0.2,0.2);
    r->setStrokeWeight(2);
    r->drawLine(beginX, y, endX, y);

    // draw slider handle
    sliderHandle_->renderPicking2DScene(r);
}

void Slider::renderPicking2DScene(ptgl::Renderer2D* r)
{
    // do nothing
    (void)r;
}

// --- SliderHandle ---
void Slider::SliderHandle::render2DScene(ptgl::Renderer2D* r)
{
    // do nothing
    (void)r;
}

void Slider::SliderHandle::renderPicking2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();
    int x = this->x();
    int y = this->y();

    double scale = (slider_->value() - slider_->minimum()) / double(slider_->maximum() - slider_->minimum());
    int pos = scale * slider_->sliderLength() + x;

    int radius = 5;
    r->setEllipseMode(ptgl::Renderer2D::Mode::Center);
    if (this->isPressed()) {
        std::array<double, 4> color{0.9,0.8,0.8,1};
        r->setFillColor(color);
    } else {
        std::array<double, 4> color{0.6,0.6,0.6,1};
        r->setFillColor(color);
    }
    r->drawCircle(pos, y, radius);
}

void Slider::SliderHandle::mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e)
{
    int x = this->x();

    int beginX = x;
    int endX = x + slider_->sliderLength();

    int vx = e->x();
    if (vx < beginX) vx = beginX;
    if (vx > endX) vx = endX;

    double range = slider_->maximum() - slider_->minimum();
    double value = range * (vx - beginX) / double(endX - beginX) + slider_->minimum();
    slider_->setValue(value);
}

}
} /* namespace ptgl */
