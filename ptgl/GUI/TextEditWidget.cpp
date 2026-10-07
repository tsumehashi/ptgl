#include "TextEditWidget.h"
#include "ptgl/Core/GraphicsItemEvent.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Core/Renderer2D.h"
#include <cmath>
namespace ptgl::gui
{
TextEditWidget::TextEditWidget()
{
    setFocusable(true);
    setSize(120, 28);
    textColor_ = theme().text;
    color_ = theme().field;
    editingColor_ = theme().editing;
}
TextEditWidget::~TextEditWidget() = default;
bool TextEditWidget::setText(const std::string &text, bool callFunc)
{
    if (editStarted_ || !utf8::valid(text) || !TextEditFilter::check(text, regexText_))
        return false;
    auto previous = text_;
    text_ = text;
    prevText_ = text_;
    textEditInfo_.setText(text_);
    if (callFunc && text_ != previous && onTextChangedFunc_)
        onTextChangedFunc_(text_, previous);
    return true;
}
const std::string &TextEditWidget::text() const
{
    return text_;
}
void TextEditWidget::setEnableEditText(bool on)
{
    enableEditText_ = on;
    if (!on)
        cancelEdit();
}
void TextEditWidget::setOnTextChangedFunction(std::function<void(const std::string &, const std::string &)> f)
{
    onTextChangedFunc_ = std::move(f);
}
void TextEditWidget::beginEdit()
{
    if (!enableEditText_ || editStarted_)
        return;
    prevText_ = text_;
    textEditInfo_.setText(text_);
    editStarted_ = true;
}
void TextEditWidget::cancelEdit()
{
    composition_.clear();
    textEditInfo_.setText(text_);
    editStarted_ = false;
    scrollX_ = 0;
}
void TextEditWidget::commitEdit()
{
    if (!editStarted_)
        return;
    editStarted_ = false;
    composition_.clear();
    finishTextEdit();
}
void TextEditWidget::cancelInteraction()
{
    Widget::cancelInteraction();
    cancelEdit();
}
void TextEditWidget::finishTextEdit()
{
    auto previous = text_;
    if (TextEditFilter::check(textEditInfo_.text(), regexText_))
        text_ = textEditInfo_.text();
    textEditInfo_.setText(text_);
    prevText_ = text_;
    scrollX_ = 0;
    if (text_ != previous && onTextChangedFunc_)
        onTextChangedFunc_(text_, previous);
}
void TextEditWidget::selectEnterEvent(GraphicsItemSelectEvent *)
{
    beginEdit();
}
void TextEditWidget::selectLeaveEvent(GraphicsItemSelectEvent *)
{
    commitEdit();
}
size_t TextEditWidget::cursorAt(int px) const
{
    double x = px - this->x() - theme().pixels(6) + scrollX_, best = 1e100;
    size_t result = 0;
    for (auto p : positions_) {
        double d = std::abs(p.second - x);
        if (d < best) {
            best = d;
            result = p.first;
        }
    }
    return result;
}
void TextEditWidget::mousePressEvent(GraphicsItemMouseEvent *e)
{
    beginEdit();
    if (!editStarted_)
        return;
    if (e->doubleClicked(MouseEvent::MouseButton::LeftButton))
        textEditInfo_.selectAll();
    else
        textEditInfo_.setCursor(cursorAt(e->x()), (e->modifierKey() & ModifierKey_Shift) != 0);
    e->setAccepted(true);
}
void TextEditWidget::mouseMoveEvent(GraphicsItemMouseEvent *e)
{
    if (editStarted_ && isPressed()) {
        textEditInfo_.setCursor(cursorAt(e->x()), true);
        e->setAccepted(true);
    }
}
void TextEditWidget::textInputEvent(const std::string &text)
{
    beginEdit();
    if (!editStarted_)
        return;
    std::string singleLine;
    for (char c : text)
        if (static_cast<unsigned char>(c) >= 32 && c != 127)
            singleLine += c;
    textEditInfo_.insert(singleLine);
    composition_.clear();
}
void TextEditWidget::textCompositionEvent(const std::string &text, int cursor)
{
    beginEdit();
    if (editStarted_) {
        composition_ = text;
        compositionCursor_ = std::clamp(cursor, 0, int(text.size()));
    }
}
void TextEditWidget::keyPressEvent(GraphicsItemKeyEvent *e)
{
    if (!enableEditText_)
        return;
    e->setAccepted(true);
    if (e->keyAction() == KeyEvent::KeyAction::KeyRelease)
        return;
    if (e->key() == Key::Key_Escape) {
        cancelEdit();
        return;
    }
    if (e->key() == Key::Key_Enter || e->key() == Key::Key_Return) {
        commitEdit();
        return;
    }
    beginEdit();
    bool shift = (e->modifierKey() & ModifierKey_Shift) != 0;
    bool ctrl = (e->modifierKey() & (ModifierKey_Control | ModifierKey_Super)) != 0;
    auto view = graphicsWindow();
    if (ctrl) {
        if (e->key() == Key::Key_A)
            textEditInfo_.selectAll();
        else if (e->key() == Key::Key_C || e->key() == Key::Key_X) {
            if (view && textEditInfo_.hasSelection())
                view->setClipboardText(textEditInfo_.selectedText());
            if (e->key() == Key::Key_X)
                textEditInfo_.eraseSelection();
        } else if (e->key() == Key::Key_V && view)
            textInputEvent(view->clipboardText());
        return;
    }
    switch (e->key()) {
    case Key::Key_BackSpace:
        textEditInfo_.deleteLeftChar();
        break;
    case Key::Key_Delete:
        textEditInfo_.deleteRightChar();
        break;
    case Key::Key_Left:
        textEditInfo_.leftCursor(shift);
        break;
    case Key::Key_Right:
        textEditInfo_.rightCursor(shift);
        break;
    case Key::Key_Home:
        textEditInfo_.homeCursor(shift);
        break;
    case Key::Key_End:
        textEditInfo_.endCursor(shift);
        break;
    default:
        if (!view || !view->hasTextInputEvents()) {
            char c;
            if (keyToAsciiChar(c, e->key(), e->modifierKey()))
                textEditInfo_.addChar(c);
        }
        break;
    }
}
void TextEditWidget::renderPicking2DScene(Renderer2D *r)
{
    r->setRectMode(Renderer2D::Mode::Corner);
    r->drawRect(x(), y(), width(), height());
}
void TextEditWidget::render2DScene(Renderer2D *r)
{
    updatePos();
    r->setRectMode(Renderer2D::Mode::Corner);
    r->setStrokeWeight(isPicked() ? 2 : 1);
    r->setStrokeColor(isPicked() ? theme().accent : theme().border);
    r->setFillColor(editStarted_ ? (customEditingColor_ ? editingColor_ : theme().editing)
                                 : (customColor_ ? color_ : theme().field));
    r->drawRect(x(), y(), width(), height());
    r->setTextSize(theme().pixels(theme().fontSize));
    const std::string &text = editStarted_ ? textEditInfo_.text() : text_;
    positions_.clear();
    for (size_t i = 0;; i = utf8::next(text, i)) {
        positions_.push_back({i, r->measureText(text.substr(0, i))});
        if (i == text.size())
            break;
    }
    const int inset = theme().pixels(6), left = x() + inset, base = y() + (height() + r->textHeight()) / 2;
    double caret = r->measureText(text.substr(0, textEditInfo_.cursorPos()));
    if (editStarted_) {
        double room = std::max(1, width() - 2 * inset);
        if (caret - scrollX_ > room)
            scrollX_ = caret - room;
        if (caret < scrollX_)
            scrollX_ = caret;
    }
    r->beginScissor(x() + 1, y() + 1, std::max(0, width() - 2), std::max(0, height() - 2));
    if (editStarted_ && textEditInfo_.hasSelection()) {
        auto a = r->measureText(text.substr(0, textEditInfo_.selectionBegin())),
             b = r->measureText(text.substr(0, textEditInfo_.selectionEnd()));
        r->setFillColor(theme().checked);
        r->setNoStroke();
        r->drawRect(int(left + a - scrollX_), y() + 2, int(b - a), height() - 4);
    }
    r->setTextColor(editStarted_ && !TextEditFilter::check(text, regexText_)
                        ? theme().error
                        : (customTextColor_ ? textColor_ : theme().text));
    if (composition_.empty())
        r->drawText(int(left - scrollX_), base, text);
    else {
        auto prefix = text.substr(0, textEditInfo_.cursorPos());
        r->drawText(int(left - scrollX_), base,
                    prefix + composition_ + text.substr(textEditInfo_.cursorPos()));
        r->setStrokeWeight(1);
        r->setStrokeColor(theme().accent);
        int cx = int(left + caret - scrollX_);
        r->drawLine(cx, base + 2, cx + int(r->measureText(composition_)), base + 2);
        caret += r->measureText(composition_.substr(0, compositionCursor_));
    }
    if (editStarted_) {
        textEditInfo_.updateTime(graphicsWindow() ? graphicsWindow()->currentTime() : 0);
        if (textEditInfo_.toggleCursorBar()) {
            r->setStrokeWeight(1);
            r->setStrokeColor(theme().text);
            int cx = int(left + caret - scrollX_);
            r->drawLine(cx, y() + 4, cx, y() + height() - 4);
        }
        if (graphicsWindow())
            graphicsWindow()->setTextInputRect(int(left + caret - scrollX_), y(), 1, height());
    }
    r->endScissor();
}
} // namespace ptgl::gui
