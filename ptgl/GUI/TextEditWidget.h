#ifndef PTGL_GUI_TEXTEDITWIDGET_H_
#define PTGL_GUI_TEXTEDITWIDGET_H_

#include <functional>
#include "Widget.h"
#include "TextEdit.h"

namespace ptgl {
namespace gui {

class TextEditWidget;
using TextEditWidgetPtr = std::shared_ptr<TextEditWidget>;

class TextEditWidget : public Widget {
public:
    TextEditWidget();
    virtual ~TextEditWidget();

    virtual bool setText(const std::string& text, bool callFunc = false);
    const std::string& text() const;

    void setFilterRegexText(const std::string& regexText) { regexText_ = regexText; }
    const std::string& filterRegexText() const { return regexText_; }

    virtual void setEnableEditText(bool enable);
    bool enableEditText() const { return enableEditText_; }

    void setTextColor(const std::array<double, 4> &color)
    {
        customTextColor_ = true;
        textColor_ = color;
    }
    void setTextColor(double r, double g, double b, double a = 1.0) { setTextColor({{r, g, b, a}}); }
    const std::array<double, 4>& textColor() const { return textColor_; }

    void setColor(const std::array<double, 4> &color)
    {
        customColor_ = true;
        color_ = color;
    }
    void setColor(double r, double g, double b, double a = 1.0) { setColor({{r, g, b, a}}); }
    const std::array<double, 4>& color() const { return color_; }

    void setEditingColor(const std::array<double, 4> &color)
    {
        customEditingColor_ = true;
        editingColor_ = color;
    }
    void setEditingColor(double r, double g, double b, double a = 1.0) { setEditingColor({{r, g, b, a}}); }
    const std::array<double, 4>& editingColor() const { return editingColor_; }

    bool isEditing() const { return editStarted_; }
    void beginEdit();
    void cancelEdit();
    void commitEdit();
    void cancelInteraction() override;

    // (currentText, prevText)
    void setOnTextChangedFunction(std::function<void (const std::string&, const std::string&)> func);

protected:
    // render
    virtual void render2DScene(ptgl::Renderer2D* r) override;
    virtual void renderPicking2DScene(ptgl::Renderer2D* r) override;

    // Mouse event
    virtual void mousePressEvent(ptgl::GraphicsItemMouseEvent* e) override;

    // Select event
    virtual void selectEnterEvent(ptgl::GraphicsItemSelectEvent* e) override;
    virtual void selectLeaveEvent(ptgl::GraphicsItemSelectEvent* e) override;

    // Key event
    virtual void keyPressEvent(ptgl::GraphicsItemKeyEvent* e) override;

    void finishTextEdit();
    void mouseMoveEvent(ptgl::GraphicsItemMouseEvent *e) override;
    void textInputEvent(const std::string &text) override;
    void textCompositionEvent(const std::string &text, int cursor) override;
    size_t cursorAt(int x) const;
    std::string composition_;
    int compositionCursor_ = 0;
    double scrollX_ = 0;
    std::vector<std::pair<size_t, double>> positions_;

    std::string prevText_;
    std::string text_;
    TextEditInfo textEditInfo_;
    bool editStarted_ = false;
    std::string regexText_;

    bool enableEditText_ = true;
    bool customTextColor_ = false, customColor_ = false, customEditingColor_ = false;

    std::array<double, 4> textColor_;
    std::array<double, 4> color_;
    std::array<double, 4> editingColor_;

    std::function<void (const std::string&, const std::string&)> onTextChangedFunc_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_TEXTEDITWIDGET_H_ */
