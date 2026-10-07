#ifndef PTGL_GUI_CONTROLS_H_
#define PTGL_GUI_CONTROLS_H_
#include "Layout.h"
#include "PushButton.h"
#include "TextEditWidget.h"
namespace ptgl::gui
{
class CheckBox : public AbstractButton
{
  public:
    explicit CheckBox(const std::string &text = "") : AbstractButton(text)
    {
        setCheckable(true);
        setSize(120, 28);
    }

  protected:
    void render2DScene(Renderer2D *r) override;
};
class DoubleSlider : public Widget
{
  public:
    DoubleSlider()
    {
        setFocusable(true);
        setEnabledWheelEvent(true);
        setSize(140, 28);
    }
    void setRange(double min, double max);
    void setValue(double value, bool notify = true);
    void setStep(double step);
    double value() const { return value_; }
    double minimum() const { return min_; }
    double maximum() const { return max_; }
    void setOnValueChangedFunction(std::function<void(double)> f) { changed_ = std::move(f); }

  protected:
    void render2DScene(Renderer2D *r) override;
    void mousePressEvent(GraphicsItemMouseEvent *e) override;
    void mouseMoveEvent(GraphicsItemMouseEvent *e) override;
    void keyPressEvent(GraphicsItemKeyEvent *e) override;
    void wheelEvent(GraphicsItemWheelEvent *e) override;
    double min_ = 0, max_ = 1, value_ = 0, step_ = .01;
    std::function<void(double)> changed_;
};
class DoubleSpinBox : public Widget
{
  public:
    DoubleSpinBox();
    void setRange(double min, double max);
    void setValue(double value, bool notify = true);
    void setStep(double step);
    void setDecimals(int decimals);
    double value() const { return value_; }
    bool isEditing() const { return edit_->isEditing(); }
    const std::shared_ptr<TextEditWidget> &editor() const { return edit_; }
    void setOnValueChangedFunction(std::function<void(double)> f) { changed_ = std::move(f); }
    void layout() override;

  protected:
    void wheelEvent(GraphicsItemWheelEvent *e) override;
    void refreshText();
    std::shared_ptr<TextEditWidget> edit_;
    PushButtonPtr up_, down_;
    double min_ = -1e9, max_ = 1e9, value_ = 0, step_ = .1;
    int decimals_ = 3;
    std::function<void(double)> changed_;
};
class ComboBox : public Widget
{
  public:
    ComboBox();
    void setItems(const std::vector<std::string> &items);
    const std::vector<std::string> &items() const { return items_; }
    void setCurrentIndex(int index, bool notify = true);
    int currentIndex() const { return index_; }
    void setOnCurrentIndexChangedFunction(std::function<void(int)> f) { changed_ = std::move(f); }
    void layout() override;
    void cancelInteraction() override;

  protected:
    void render2DScene(Renderer2D *r) override;
    void mousePressEvent(GraphicsItemMouseEvent *e) override;
    void keyPressEvent(GraphicsItemKeyEvent *e) override;
    std::vector<std::string> items_;
    int index_ = -1;
    std::shared_ptr<Panel> popup_;
    std::function<void(int)> changed_;
};
class ColorPicker : public Widget
{
  public:
    ColorPicker();
    void setColor(const std::array<double, 3> &color, bool notify = true);
    const std::array<double, 3> &color() const { return color_; }
    void setOnColorChangedFunction(std::function<void(const std::array<double, 3> &)> f)
    {
        changed_ = std::move(f);
    }
    void layout() override;
    void cancelInteraction() override;

  protected:
    void render2DScene(Renderer2D *r) override;
    void mousePressEvent(GraphicsItemMouseEvent *e) override;
    void keyPressEvent(GraphicsItemKeyEvent *e) override;
    std::array<double, 3> color_{{1, 1, 1}};
    std::shared_ptr<Panel> popup_;
    std::array<std::shared_ptr<DoubleSlider>, 3> sliders_;
    std::function<void(const std::array<double, 3> &)> changed_;
};
} // namespace ptgl::gui
#endif
