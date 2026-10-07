#include "Slider.h"
#include "ptgl/Core/GraphicsItemEvent.h"
#include "ptgl/Core/Renderer2D.h"
#include <stdexcept>

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
    if (sliderLength < 0)
        throw std::invalid_argument("Negative slider length");
    sliderLength_ = sliderLength;
}

void Slider::layout()
{
    updatePos();
    double range = double(maximum()) - minimum();
    int offset = range > 0 ? int((double(value()) - minimum()) / range * sliderLength_) : 0;
    sliderHandle_->setSize(10, 10);
    sliderHandle_->setLocalPos(offset - 5, -5);
    sliderHandle_->updatePos();
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

    int pos = x + width() / 2;
    y += height() / 2;

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
    int x = slider_->x();

    int beginX = x;
    int endX = x + slider_->sliderLength();

    int vx = e->x();
    if (vx < beginX) vx = beginX;
    if (vx > endX) vx = endX;

    if (endX <= beginX)
        return;
    double range = double(slider_->maximum()) - slider_->minimum();
    double value = range * (vx - beginX) / double(endX - beginX) + slider_->minimum();
    slider_->setValue(value);
}

}
} /* namespace ptgl */
