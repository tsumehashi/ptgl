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
    customColor_ = true;
    color_ = color;
    return *this;
}

PushButton& PushButton::setColor(double r, double g, double b, double a)
{
    customColor_ = true;
    color_ = {{r, g, b, a}};
    return *this;
}

PushButton& PushButton::setCheckedColor(const std::array<double, 4>& color)
{
    customCheckedColor_ = true;
    checkedColor_ = color;
    return *this;
}

PushButton& PushButton::setCheckedColor(double r, double g, double b, double a)
{
    customCheckedColor_ = true;
    checkedColor_ = {{r, g, b, a}};
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
    if (!isVisible())
        return;

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    r->setStrokeColor(isPicked() ? theme().accent : theme().border);
    r->setStrokeWeight(isPicked() ? 2 : 1);

    if (isDown() || isChecked()) {
        r->setFillColor(customCheckedColor_ ? checkedColor_ : theme().checked);
    } else {
        r->setFillColor(customColor_ ? color_ : theme().field);
    }
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);

    // render text
    r->setTextColor(isEnabled() ? theme().text : theme().muted);

    int textSize = std::min(height() - 2, theme().pixels(theme().fontSize));
    int dy = (this->height() - textSize)/2;
    r->setTextSize(textSize);
    r->beginScissor(x + 2, y + 1, std::max(0, w - 4), std::max(0, h - 2));
    r->drawText(x+4, y + this->height() - dy, text());
    r->endScissor();
}

}
} /* namespace ptgl */
