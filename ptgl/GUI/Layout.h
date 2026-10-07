#ifndef PTGL_GUI_LAYOUT_H_
#define PTGL_GUI_LAYOUT_H_
#include "Widget.h"
#include <vector>
namespace ptgl::gui
{
class Label : public Widget
{
  public:
    explicit Label(std::string text = "") : text_(std::move(text)) { setPickable(false); }
    void setText(const std::string &text) { text_ = text; }
    const std::string &text() const { return text_; }

  protected:
    void render2DScene(Renderer2D *r) override { drawCaption(r, text_, 0); }
    std::string text_;
};
class Layout : public Widget
{
  public:
    enum class Direction { Vertical, Horizontal };
    explicit Layout(Direction direction = Direction::Vertical) : direction_(direction) {}
    void add(WidgetPtr widget, double stretch = 0);
    void addWidget(WidgetPtr widget) override { add(std::move(widget)); }
    void removeWidget(WidgetPtr widget) override;
    void clear();
    void setPadding(int padding);
    void setSpacing(int spacing);
    void layout() override;
    std::array<int, 2> sizeHint() const override;

  protected:
    struct Entry {
        WidgetPtr widget;
        double stretch;
    };
    std::vector<Entry> entries_;
    Direction direction_;
    int padding_ = -1, spacing_ = -1;
    virtual int headerHeight() const { return 0; }
};
class FormLayout : public Layout
{
  public:
    void addRow(const std::string &caption, WidgetPtr editor);
};
class Panel : public Layout
{
  public:
    explicit Panel(std::string title = "") : title_(std::move(title)) { setClipChildren(true); }

  protected:
    int headerHeight() const override { return title_.empty() ? 0 : theme().pixels(theme().rowHeight); }
    void render2DScene(Renderer2D *r) override;
    std::string title_;
};
class ScrollArea : public Widget
{
  public:
    ScrollArea()
    {
        setClipChildren(true);
        setEnabledWheelEvent(true);
    }
    void setContent(WidgetPtr content);
    const WidgetPtr &content() const { return content_; }
    void setScrollOffset(int offset);
    int scrollOffset() const { return offset_; }
    void ensureVisible(const Widget &child);
    void layout() override;

  protected:
    void render2DScene(Renderer2D *r) override;
    void wheelEvent(GraphicsItemWheelEvent *e) override;
    void mousePressEvent(GraphicsItemMouseEvent *e) override;
    void mouseMoveEvent(GraphicsItemMouseEvent *e) override;
    WidgetPtr content_;
    int offset_ = 0, maximum_ = 0;
};
} // namespace ptgl::gui
#endif
