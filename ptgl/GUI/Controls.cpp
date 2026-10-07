#include "Controls.h"
#include "ptgl/Core/GraphicsItemEvent.h"
#include "ptgl/Core/Renderer2D.h"
#include <iomanip>
#include <sstream>
#include <locale>
namespace ptgl::gui
{
void CheckBox::render2DScene(Renderer2D *r)
{
    int size = theme().pixels(16), top = y() + (height() - size) / 2;
    r->setRectMode(Renderer2D::Mode::Corner);
    r->setStrokeWeight(isPicked() ? 2 : 1);
    r->setStrokeColor(isPicked() ? theme().accent : theme().border);
    r->setFillColor(isChecked() ? theme().checked : theme().field);
    r->drawRect(x(), top, size, size);
    if (isChecked()) {
        r->setStrokeColor(theme().text);
        r->setStrokeWeight(2);
        r->drawLine(x() + 3, top + size / 2, x() + size / 2, top + size - 4);
        r->drawLine(x() + size / 2, top + size - 4, x() + size - 3, top + 3);
    }
    drawCaption(r, text_, 24);
}
void DoubleSlider::setRange(double min, double max)
{
    if (!std::isfinite(min) || !std::isfinite(max) || min > max || !std::isfinite(max - min))
        throw std::invalid_argument("Invalid slider range");
    min_ = min;
    max_ = max;
    setValue(value_);
}
void DoubleSlider::setStep(double s)
{
    if (!std::isfinite(s) || s <= 0)
        throw std::invalid_argument("Invalid step");
    step_ = s;
}
void DoubleSlider::setValue(double v, bool notify)
{
    if (!std::isfinite(v))
        throw std::invalid_argument("Invalid value");
    v = std::clamp(v, min_, max_);
    if (v == value_)
        return;
    value_ = v;
    auto f = changed_;
    if (notify && f)
        f(v);
}
void DoubleSlider::render2DScene(Renderer2D *r)
{
    drawFrame(r);
    int pad = theme().pixels(8), w = std::max(0, width() - 2 * pad), cy = y() + height() / 2;
    r->setStrokeWeight(3);
    r->setStrokeColor(theme().border);
    r->drawLine(x() + pad, cy, x() + pad + w, cy);
    double fraction = max_ > min_ ? (value_ - min_) / (max_ - min_) : 0;
    r->setStrokeColor(theme().accent);
    r->drawLine(x() + pad, cy, x() + pad + int(w * fraction), cy);
    r->setFillColor(theme().text);
    r->setNoStroke();
    r->setEllipseMode(Renderer2D::Mode::Center);
    r->drawCircle(x() + pad + int(w * fraction), cy, theme().pixels(5));
}
void DoubleSlider::mousePressEvent(GraphicsItemMouseEvent *e)
{
    mouseMoveEvent(e);
}
void DoubleSlider::mouseMoveEvent(GraphicsItemMouseEvent *e)
{
    int pad = theme().pixels(8), w = width() - 2 * pad;
    if (w > 0) {
        double f = std::clamp(double(e->x() - x() - pad) / w, 0., 1.);
        setValue(min_ + (max_ - min_) * f);
    }
    e->setAccepted(true);
}
void DoubleSlider::wheelEvent(GraphicsItemWheelEvent *e)
{
    if (e->delta())
        setValue(value_ + (e->delta() > 0 ? step_ : -step_));
    e->setAccepted(true);
}
void DoubleSlider::keyPressEvent(GraphicsItemKeyEvent *e)
{
    if (e->key() != Key::Key_Left && e->key() != Key::Key_Right && e->key() != Key::Key_Up &&
        e->key() != Key::Key_Down && e->key() != Key::Key_Home && e->key() != Key::Key_End)
        return;
    e->setAccepted(true);
    if (e->keyAction() == KeyEvent::KeyAction::KeyRelease)
        return;
    if (e->key() == Key::Key_Home)
        setValue(min_);
    else if (e->key() == Key::Key_End)
        setValue(max_);
    else
        setValue(value_ + ((e->key() == Key::Key_Right || e->key() == Key::Key_Up) ? 1 : -1) * step_ *
                              ((e->modifierKey() & ModifierKey_Shift) ? 10 : 1));
}
namespace
{
class NumericEdit : public TextEditWidget
{
  public:
    std::function<void(int)> step;

  protected:
    void keyPressEvent(GraphicsItemKeyEvent *e) override
    {
        if (e->key() == Key::Key_Up || e->key() == Key::Key_Down) {
            e->setAccepted(true);
            if (e->keyAction() != KeyEvent::KeyAction::KeyRelease) {
                commitEdit();
                step(e->key() == Key::Key_Up ? 1 : -1);
            }
        } else
            TextEditWidget::keyPressEvent(e);
    }
};
} // namespace
DoubleSpinBox::DoubleSpinBox()
{
    setSize(140, 28);
    setEnabledWheelEvent(true);
    auto edit = std::make_shared<NumericEdit>();
    edit_ = edit;
    edit->step = [this](int sign) { setValue(value_ + sign * step_); };
    up_ = std::make_shared<PushButton>("+");
    down_ = std::make_shared<PushButton>("-");
    addWidget(edit_);
    addWidget(down_);
    addWidget(up_);
    up_->setOnClickedFunction([this](bool) {
        edit_->commitEdit();
        setValue(value_ + step_);
    });
    down_->setOnClickedFunction([this](bool) {
        edit_->commitEdit();
        setValue(value_ - step_);
    });
    edit_->setOnTextChangedFunction([this](const std::string &s, const std::string &) {
        std::istringstream input(s);
        input.imbue(std::locale::classic());
        double v;
        if ((input >> v) && (input >> std::ws).eof() && std::isfinite(v))
            setValue(v);
        refreshText();
    });
    refreshText();
}
void DoubleSpinBox::setRange(double min, double max)
{
    if (!std::isfinite(min) || !std::isfinite(max) || min > max)
        throw std::invalid_argument("Invalid spin range");
    min_ = min;
    max_ = max;
    setValue(value_);
}
void DoubleSpinBox::setStep(double s)
{
    if (!std::isfinite(s) || s <= 0)
        throw std::invalid_argument("Invalid step");
    step_ = s;
}
void DoubleSpinBox::setDecimals(int d)
{
    if (d < 0 || d > 12)
        throw std::invalid_argument("Invalid decimal count");
    decimals_ = d;
    refreshText();
}
void DoubleSpinBox::refreshText()
{
    std::ostringstream s;
    s.imbue(std::locale::classic());
    s << std::fixed << std::setprecision(decimals_) << value_;
    edit_->setText(s.str());
}
void DoubleSpinBox::setValue(double v, bool notify)
{
    if (!std::isfinite(v))
        throw std::invalid_argument("Invalid numeric value");
    v = std::clamp(v, min_, max_);
    bool changed = v != value_;
    value_ = v;
    refreshText();
    auto f = changed_;
    if (changed && notify && f)
        f(v);
}
void DoubleSpinBox::layout()
{
    updatePos();
    int b = std::min(width() / 3, theme().pixels(22));
    edit_->setLocalPos(0, 0);
    edit_->setSize(std::max(0, width() - 2 * b), height());
    down_->setLocalPos(width() - 2 * b, 0);
    down_->setSize(b, height());
    up_->setLocalPos(width() - b, 0);
    up_->setSize(b, height());
}
void DoubleSpinBox::wheelEvent(GraphicsItemWheelEvent *e)
{
    edit_->commitEdit();
    if (e->delta())
        setValue(value_ + (e->delta() > 0 ? step_ : -step_));
    e->setAccepted(true);
}
ComboBox::ComboBox()
{
    setFocusable(true);
    setSize(140, 28);
    popup_ = std::make_shared<Panel>();
    popup_->setPopup(true);
    popup_->setPadding(3);
    popup_->setSpacing(2);
    popup_->setVisible(false);
    addWidget(popup_);
}
void ComboBox::setItems(const std::vector<std::string> &items)
{
    if (items_ == items)
        return;
    items_ = items;
    popup_->clear();
    for (size_t i = 0; i < items.size(); ++i) {
        auto b = std::make_shared<PushButton>(items[i]);
        b->setPreferredSize(120, 28);
        b->setOnClickedFunction([this, i](bool) {
            popup_->setVisible(false);
            setCurrentIndex(int(i));
        });
        popup_->add(b);
    }
    index_ = items.empty() ? -1 : std::clamp(index_, 0, int(items.size()) - 1);
}
void ComboBox::setCurrentIndex(int i, bool notify)
{
    if (i < -1 || i >= int(items_.size()))
        throw std::out_of_range("Combo index");
    if (i == index_)
        return;
    index_ = i;
    auto f = changed_;
    if (notify && f)
        f(i);
}
void ComboBox::layout()
{
    updatePos();
    int h = popup_->sizeHint()[1], top = height();
    if (graphicsWindow() && y() + top + h > viewHeight())
        top = -h;
    popup_->setLocalPos(0, top);
    popup_->setSize(width(), h);
}
void ComboBox::render2DScene(Renderer2D *r)
{
    drawFrame(r);
    drawCaption(r, index_ >= 0 ? items_[index_] : "(none)");
    r->setTextColor(theme().muted);
    r->drawText(x() + width() - theme().pixels(16), y() + (height() + r->textHeight()) / 2, "v");
}
void ComboBox::mousePressEvent(GraphicsItemMouseEvent *e)
{
    popup_->setVisible(!popup_->isLocallyVisible() && !items_.empty());
    e->setAccepted(true);
}
void ComboBox::cancelInteraction()
{
    Widget::cancelInteraction();
    popup_->setVisible(false);
}
void ComboBox::keyPressEvent(GraphicsItemKeyEvent *e)
{
    e->setAccepted(true);
    if (e->keyAction() == KeyEvent::KeyAction::KeyRelease)
        return;
    if (e->key() == Key::Key_Escape)
        popup_->setVisible(false);
    else if (e->key() == Key::Key_Space || e->key() == Key::Key_Enter)
        popup_->setVisible(!popup_->isLocallyVisible());
    else if (!items_.empty() && (e->key() == Key::Key_Down || e->key() == Key::Key_Up))
        setCurrentIndex(std::clamp(index_ + (e->key() == Key::Key_Down ? 1 : -1), 0, int(items_.size()) - 1));
}
ColorPicker::ColorPicker()
{
    setFocusable(true);
    setSize(140, 28);
    popup_ = std::make_shared<Panel>("RGB");
    popup_->setPopup(true);
    popup_->setVisible(false);
    addWidget(popup_);
    auto form = std::make_shared<FormLayout>();
    form->setPadding(0);
    for (int i = 0; i < 3; ++i) {
        sliders_[i] = std::make_shared<DoubleSlider>();
        sliders_[i]->setValue(1, false);
        sliders_[i]->setOnValueChangedFunction([this, i](double value) {
            auto c = color_;
            c[i] = value;
            setColor(c);
        });
        form->addRow(i == 0 ? "Red" : i == 1 ? "Green" : "Blue", sliders_[i]);
    }
    popup_->add(form);
}
void ColorPicker::setColor(const std::array<double, 3> &c, bool notify)
{
    for (double v : c)
        if (!std::isfinite(v) || v < 0 || v > 1)
            throw std::invalid_argument("Invalid RGB");
    bool changed = c != color_;
    color_ = c;
    for (int i = 0; i < 3; ++i)
        sliders_[i]->setValue(c[i], false);
    auto f = changed_;
    if (changed && notify && f)
        f(c);
}
void ColorPicker::layout()
{
    updatePos();
    int h = popup_->sizeHint()[1], w = std::max(width(), theme().pixels(260)), top = height(), left = 0;
    if (graphicsWindow()) {
        if (y() + top + h > viewHeight())
            top = -h;
        left = std::min(0, viewWidth() - x() - w);
    }
    popup_->setLocalPos(left, top);
    popup_->setSize(w, h);
}
void ColorPicker::render2DScene(Renderer2D *r)
{
    drawFrame(r);
    r->setFillColor(color_);
    r->setNoStroke();
    r->drawRect(x() + 3, y() + 3, std::max(0, width() - 6), std::max(0, height() - 6));
}
void ColorPicker::mousePressEvent(GraphicsItemMouseEvent *e)
{
    popup_->setVisible(!popup_->isLocallyVisible());
    e->setAccepted(true);
}
void ColorPicker::keyPressEvent(GraphicsItemKeyEvent *e)
{
    e->setAccepted(true);
    if (e->keyAction() != KeyEvent::KeyAction::KeyPress)
        return;
    if (e->key() == Key::Key_Escape)
        popup_->setVisible(false);
    else if (e->key() == Key::Key_Space || e->key() == Key::Key_Enter)
        popup_->setVisible(!popup_->isLocallyVisible());
}
void ColorPicker::cancelInteraction()
{
    Widget::cancelInteraction();
    popup_->setVisible(false);
}
} // namespace ptgl::gui
