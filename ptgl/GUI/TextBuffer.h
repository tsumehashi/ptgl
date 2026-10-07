#ifndef PTGL_GUI_TEXTBUFFER_H_
#define PTGL_GUI_TEXTBUFFER_H_
#include "Utf8.h"
#include <algorithm>
#include <stdexcept>
namespace ptgl::gui
{
// Cursor/anchor are UTF-8 byte offsets, always at a codepoint boundary.
class TextEditInfo
{
  public:
    void setText(const std::string &text)
    {
        if (!utf8::valid(text))
            throw std::invalid_argument("Invalid UTF-8");
        text_ = text;
        cursor_ = anchor_ = text.size();
    }
    void clear() { setText(""); }
    void setCursor(size_t p, bool extend = false)
    {
        p = std::min(p, text_.size());
        while (p > 0 && p < text_.size() && (static_cast<unsigned char>(text_[p]) & 0xc0) == 0x80)
            --p;
        cursor_ = p;
        if (!extend)
            anchor_ = p;
    }
    void selectAll()
    {
        anchor_ = 0;
        cursor_ = text_.size();
    }
    size_t selectionBegin() const { return std::min(anchor_, cursor_); }
    size_t selectionEnd() const { return std::max(anchor_, cursor_); }
    bool hasSelection() const { return anchor_ != cursor_; }
    std::string selectedText() const
    {
        return text_.substr(selectionBegin(), selectionEnd() - selectionBegin());
    }
    void eraseSelection()
    {
        size_t a = selectionBegin();
        text_.erase(a, selectionEnd() - a);
        cursor_ = anchor_ = a;
    }
    void insert(const std::string &s)
    {
        if (!utf8::valid(s))
            return;
        eraseSelection();
        text_.insert(cursor_, s);
        cursor_ += s.size();
        anchor_ = cursor_;
    }
    void addChar(char c)
    {
        if (static_cast<unsigned char>(c) < 128)
            insert(std::string(1, c));
    }
    void deleteLeftChar()
    {
        if (hasSelection())
            eraseSelection();
        else if (cursor_) {
            anchor_ = utf8::previous(text_, cursor_);
            eraseSelection();
        }
    }
    void deleteRightChar()
    {
        if (hasSelection())
            eraseSelection();
        else if (cursor_ < text_.size()) {
            anchor_ = utf8::next(text_, cursor_);
            eraseSelection();
        }
    }
    void leftCursor(bool extend = false)
    {
        setCursor(!extend && hasSelection() ? selectionBegin() : utf8::previous(text_, cursor_), extend);
    }
    void rightCursor(bool extend = false)
    {
        setCursor(!extend && hasSelection() ? selectionEnd() : utf8::next(text_, cursor_), extend);
    }
    void homeCursor(bool extend = false) { setCursor(0, extend); }
    void endCursor(bool extend = false) { setCursor(text_.size(), extend); }
    void setCursorBarToggleTime(double t)
    {
        if (t > 0)
            deltaTime_ = t;
    }
    void updateTime(double t)
    {
        if (t - prevTime_ >= deltaTime_) {
            toggleBar_ = !toggleBar_;
            prevTime_ = t;
        }
    }
    const std::string &text() const { return text_; }
    int cursorPos() const { return int(cursor_); }
    bool toggleCursorBar() const { return toggleBar_; }

  private:
    std::string text_;
    size_t cursor_ = 0, anchor_ = 0;
    double deltaTime_ = .5, prevTime_ = 0;
    bool toggleBar_ = true;
};
} // namespace ptgl::gui
#endif
