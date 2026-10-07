#include "Layout.h"
#include "ptgl/Core/Renderer2D.h"
#include "ptgl/Core/GraphicsItemEvent.h"
namespace ptgl::gui
{
void Layout::add(WidgetPtr w, double stretch)
{
    if (!w)
        return;
    if (!std::isfinite(stretch) || stretch < 0)
        throw std::invalid_argument("Invalid stretch");
    for (auto &e : entries_)
        if (e.widget == w)
            return;
    Widget::addWidget(w);
    entries_.push_back({w, stretch});
}
void Layout::removeWidget(WidgetPtr w)
{
    Widget::removeWidget(w);
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(), [&](const Entry &e) { return e.widget == w; }),
        entries_.end());
}
void Layout::clear()
{
    auto entries = entries_;
    for (auto &e : entries)
        removeWidget(e.widget);
}
void Layout::setPadding(int p)
{
    if (p < 0)
        throw std::invalid_argument("Negative padding");
    padding_ = p;
}
void Layout::setSpacing(int p)
{
    if (p < 0)
        throw std::invalid_argument("Negative spacing");
    spacing_ = p;
}
std::array<int, 2> Layout::sizeHint() const
{
    int pad = theme().pixels(padding_ < 0 ? theme().padding : padding_),
        gap = theme().pixels(spacing_ < 0 ? theme().spacing : spacing_);
    int main = 0, cross = 0, count = 0, axis = direction_ == Direction::Vertical ? 1 : 0;
    for (auto &e : entries_)
        if (e.widget->isLocallyVisible()) {
            auto h = e.widget->sizeHint();
            main += h[axis];
            cross = std::max(cross, h[1 - axis]);
            ++count;
        }
    main += std::max(0, count - 1) * gap;
    std::array<int, 2> result;
    result[axis] = main + 2 * pad;
    result[1 - axis] = cross + 2 * pad;
    result[1] += headerHeight();
    auto min = minimumSize();
    for (int i = 0; i < 2; ++i)
        result[i] = std::max(result[i], min[i]);
    return result;
}
void Layout::layout()
{
    updatePos();
    int pad = theme().pixels(padding_ < 0 ? theme().padding : padding_),
        gap = theme().pixels(spacing_ < 0 ? theme().spacing : spacing_);
    int axis = direction_ == Direction::Vertical ? 1 : 0,
        space = (axis ? height() - headerHeight() : width()) - 2 * pad;
    int used = 0, count = 0;
    double stretch = 0;
    for (auto &e : entries_)
        if (e.widget->isLocallyVisible()) {
            used += e.widget->sizeHint()[axis];
            stretch += e.stretch;
            ++count;
        }
    int extra = std::max(0, space - used - std::max(0, count - 1) * gap),
        pos = pad + (axis ? headerHeight() : 0);
    for (auto &e : entries_)
        if (e.widget->isLocallyVisible()) {
            int length = e.widget->sizeHint()[axis] + (stretch > 0 ? int(extra * e.stretch / stretch) : 0);
            int cross = std::max(0, (axis ? width() : height() - headerHeight()) - 2 * pad);
            e.widget->setLocalPos(axis ? pad : pos, axis ? pos : pad + headerHeight());
            e.widget->setSize(axis ? cross : length, axis ? length : cross);
            e.widget->updatePos();
            pos += length + gap;
        }
}
void FormLayout::addRow(const std::string &caption, WidgetPtr editor)
{
    auto row = std::make_shared<Layout>(Direction::Horizontal);
    row->setPadding(0);
    auto label = std::make_shared<Label>(caption);
    label->setPreferredSize(90, 28);
    editor->setPreferredSize(130, 28);
    row->add(label);
    row->add(editor, 1);
    add(row);
}
void Panel::render2DScene(Renderer2D *r)
{
    r->setRectMode(Renderer2D::Mode::Corner);
    r->setStrokeWeight(1);
    r->setStrokeColor(theme().border);
    r->setFillColor(theme().background);
    int header = headerHeight();
    r->drawRect(x(), y() + header, width(), std::max(0, height() - header));
    if (header > 0) {
        r->setFillColor(theme().titleBackground);
        r->drawRect(x(), y(), width(), header);
    }
    r->setTextSize(theme().pixels(theme().fontSize));
    r->setTextColor(theme().titleText);
    r->drawText(x() + theme().pixels(theme().padding), y() + theme().pixels(theme().fontSize + 6), title_);
}
void ScrollArea::setContent(WidgetPtr w)
{
    if (content_ == w)
        return;
    if (content_)
        removeWidget(content_);
    content_ = w;
    if (w)
        addWidget(w);
    offset_ = 0;
}
void ScrollArea::setScrollOffset(int offset)
{
    offset_ = std::clamp(offset, 0, maximum_);
    layout();
}
void ScrollArea::layout()
{
    updatePos();
    if (!content_)
        return;
    int h = std::max(height(), content_->sizeHint()[1]);
    maximum_ = h - height();
    offset_ = std::clamp(offset_, 0, maximum_);
    content_->setLocalPos(0, -offset_);
    content_->setSize(std::max(0, width() - theme().pixels(10)), h);
    content_->updatePos();
}
void ScrollArea::ensureVisible(const Widget &child)
{
    if (child.y() < y())
        setScrollOffset(offset_ + child.y() - y());
    else if (child.y() + child.height() > y() + height())
        setScrollOffset(offset_ + child.y() + child.height() - y() - height());
}
void ScrollArea::wheelEvent(GraphicsItemWheelEvent *e)
{
    setScrollOffset(offset_ - e->delta() / 2);
    e->setAccepted(true);
}
void ScrollArea::mousePressEvent(GraphicsItemMouseEvent *e)
{
    mouseMoveEvent(e);
}
void ScrollArea::mouseMoveEvent(GraphicsItemMouseEvent *e)
{
    if (e->x() >= x() + width() - theme().pixels(12) && height() > 0) {
        setScrollOffset(int(double(e->y() - y()) / height() * (maximum_ + height()) - height() / 2));
        e->setAccepted(true);
    }
}
void ScrollArea::render2DScene(Renderer2D *r)
{
    if (maximum_ <= 0)
        return;
    int thumb = std::max(theme().pixels(18), int(double(height()) * height() / (height() + maximum_))),
        top = int(double(offset_) / maximum_ * (height() - thumb));
    r->setRectMode(Renderer2D::Mode::Corner);
    r->setNoStroke();
    r->setFillColor(theme().border);
    r->drawRect(x() + width() - theme().pixels(7), y() + top, theme().pixels(5), thumb);
}
} // namespace ptgl::gui
