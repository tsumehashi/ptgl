#include "ptgl/Core/Renderer2D.h"
#include "PushButton.h"

namespace ptgl {
namespace gui {

PushButton::PushButton()
{
    setSize(100, 20);
    color_ = {{1,1,1,0.9}};
    checkedColor_ = {{1.0, 0.8, 0.8, 1.0}};
}

PushButton::PushButton(const std::string& text)
    : AbstractButton(text)
{
    setSize(100, 20);
    color_ = {{1,1,1,0.9}};
    checkedColor_ = {{1.0, 0.8, 0.8, 1.0}};

    setText(text);
}

PushButton::~PushButton()
{

}

PushButton& PushButton::setColor(const std::array<double, 4>& color)
{
    color_ = color;
    return *this;
}

PushButton& PushButton::setColor(double r, double g, double b, double a)
{
    color_ = {{r,g,b,a}};
    return *this;
}

PushButton& PushButton::setCheckedColor(const std::array<double, 4>& color)
{
    checkedColor_ = color;
    return *this;
}

PushButton& PushButton::setCheckedColor(double r, double g, double b, double a)
{
    checkedColor_ = {{r,g,b,a}};
    return *this;
}

AbstractButton& PushButton::setText(const std::string& text)
{
    AbstractButton::setText(text);

    // auto resize
    int nw = this->height() * text_.size()*0.6;
    nw = std::max(nw, this->width());
    this->setWidth(nw);
    return *this;
}

void PushButton::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);

    if (isDown() || isChecked()) {
        r->setFillColor(checkedColor_);
    } else {
        r->setFillColor(color_);
    }
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);

    // render text
    r->setTextColor(0,0,0);

    int textSize = this->height() * 0.9;
    int dy = (this->height() - textSize)/2;
    r->setTextSize(textSize);
    r->drawText(x+4, y + this->height() - dy, text());
}

}
} /* namespace ptgl */
