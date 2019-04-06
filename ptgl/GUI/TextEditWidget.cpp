#include "TextEditWidget.h"
#include "ptgl/Core/Common.h"
#include "ptgl/Core/GraphicsItemEvent.h"
#include "ptgl/Core/GraphicsView.h"
#include "ptgl/Core/Renderer2D.h"

namespace ptgl {
namespace gui {

TextEditWidget::TextEditWidget()
{
    setEnabledKeyEvent(true);
    setSize(40, 20);
    textColor_ = {{0,0,0,0.9}};
    color_ = {{1,1,1,0.9}};
    editingColor_ = {{0.8, 0.8, 1.0, 1.0}};
}

TextEditWidget::~TextEditWidget() {

}

bool TextEditWidget::setText(const std::string& text, bool callFunc)
{
    if (!editStarted_) {
        // check filter
        if (!TextEditFilter::check(text, regexText_)) {
            return false;
        }

        prevText_ = text_;
        text_ = text;
        textEditInfo_.setText(text_);
        if (callFunc && onTextChangedFunc_) {
            onTextChangedFunc_(this->text(), prevText_);
        }
    }

    return false;
}

const std::string& TextEditWidget::text() const
{
    return text_;
}

void TextEditWidget::setEnableEditText(bool enable)
{
    enableEditText_ = enable;
    // stop edit and restore
    if (!enable && editStarted_) {
        textEditInfo_.setText(text_);
        editStarted_ = false;
    }
}

// (currentText, prevText)
void TextEditWidget::setOnTextChangedFunction(std::function<void (const std::string&, const std::string&)> func)
{
    onTextChangedFunc_ = func;
}

void TextEditWidget::render2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    this->updatePos();

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    int textSize = h * 0.9;
    r->setTextSize(textSize);
    int textOffsetX = 3;

    // draw text box
    r->setStrokeColor(0.3, 0.3, 0.3, 0.8);
    r->setStrokeWeight(1);

    int boxW = w;
    if (editStarted_) {
        if (TextEditFilter::check(textEditInfo_.text(), regexText_)) {
            // valid
            r->setFillColor(editingColor_);
        } else {
            // invalid
            r->setFillColor(1.0, 0.8, 0.8);
        }
        boxW = std::max(boxW, r->textWidth(textEditInfo_.text().size())+textOffsetX);
    } else {
        r->setFillColor(color_);
    }

    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,boxW,h);

    // draw text
    int tx = x + textOffsetX;
    int ty = y + h - (h - r->textHeight()) * 0.5;

    if (editStarted_) {
        // draw cursor bar
        if (textEditInfo_.toggleCursorBar()) {
            r->setStrokeColor(0,0,0);
        } else {
            r->setStrokeColor(1,1,1);
        }
        textEditInfo_.updateTime(r->graphicsView()->currentTime());

        r->setStrokeWeight(1);
        int px = tx + r->textWidth(textEditInfo_.cursorPos());
        int py1 = ty;
        int py2 = ty - r->textHeight();
        r->drawLine(px, py1, px, py2);

        // draw editing text
        r->setTextColor(textColor_);
        r->drawText(tx,ty, textEditInfo_.text());
    } else {
        // draw text
        r->setTextColor(textColor_);
        r->drawText(tx,ty, text());
    }
}

void TextEditWidget::renderPicking2DScene(ptgl::Renderer2D* r)
{
    if (!isEnabled() || !isVisible()) return;

    int x = this->x();
    int y = this->y();

    int w = this->width();
    int h = this->height();

    // draw background
    r->setStrokeWeight(1);
    r->setRectMode(ptgl::Renderer2D::Mode::Corner);
    r->drawRect(x,y,w,h);
}

void TextEditWidget::mousePressEvent(ptgl::GraphicsItemMouseEvent* e)
{
    (void)e;

    if (!enableEditText_) {
        return;
    }

    if (e->doubleClicked(ptgl::MouseEvent::MouseButton::LeftButton) && !editStarted_) {
        // edit start
        textEditInfo_.setText(text_);
        editStarted_ = true;
    }
}

void TextEditWidget::selectEnterEvent(ptgl::GraphicsItemSelectEvent* e)
{
    (void)e;

#if 0
    if (!enableEditText_) {
        return;
    }

    if (!editStarted_) {
        // edit start
        textEditInfo_.setText(text_);
        editStarted_ = true;
    }
#endif
}

void TextEditWidget::selectLeaveEvent(ptgl::GraphicsItemSelectEvent* e)
{
    (void)e;

    if (!enableEditText_) {
        return;
    }

    if (editStarted_) {
        editStarted_ = false;
        finishTextEdit();
    }
}

void TextEditWidget::keyPressEvent(ptgl::GraphicsItemKeyEvent* e)
{
    if (!enableEditText_) {
        return;
    }
    if (editStarted_) {
        ptgl::Key key = e->key();
        char result = 0;
        if (ptgl::keyToAsciiChar(result, key, e->modifierKey())) {
            textEditInfo_.addChar(result);
        } else {
            if (key == ptgl::Key::Key_BackSpace) {
                textEditInfo_.deleteLeftChar();
            } else if (key == ptgl::Key::Key_Delete) {
                textEditInfo_.deleteRightChar();
            } else if (key == ptgl::Key::Key_Right) {
                textEditInfo_.rightCursor();
            } else if (key == ptgl::Key::Key_Left) {
                textEditInfo_.leftCursor();
            } else if (key == ptgl::Key::Key_Home) {
                textEditInfo_.homeCursor();
            } else if (key == ptgl::Key::Key_End) {
                textEditInfo_.endCursor();
            } else if (key == ptgl::Key::Key_Enter) {
                editStarted_ = false;
                finishTextEdit();
            }
        }

        e->setAccepted(true);
    }
}

void TextEditWidget::finishTextEdit()
{
    // check validate
    if (TextEditFilter::check(textEditInfo_.text(), regexText_)) {
        // valid
        text_ = textEditInfo_.text();
    } else {
        // invalid
        textEditInfo_.setText(text_);    // restore
    }

    if (onTextChangedFunc_) {
        onTextChangedFunc_(this->text(), prevText_);
    }
    prevText_ = text_;
}

}
} /* namespace ptgl */
