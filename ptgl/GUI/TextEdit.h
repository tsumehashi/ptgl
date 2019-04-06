#ifndef PTGL_GUI_TEXTEDIT_H_
#define PTGL_GUI_TEXTEDIT_H_

#include <string>

namespace ptgl {
namespace gui {

class TextEditInfo {
public:

    TextEditInfo() {}
    virtual ~TextEditInfo() {}

    void setText(const std::string& text) {
        text_ = text;
        cursorPos_ = text_.size();
    }

    void clear() {
        text_.clear();
        cursorPos_ = 0;
    }

    void addChar(char c) {
        text_.insert(cursorPos_, std::string{c});
        ++cursorPos_;
    }

    void deleteLeftChar() {
        if ((cursorPos_ > 0) && !text_.empty()) {
            text_.erase(cursorPos_-1, 1);
            --cursorPos_;
        }
    }

    void deleteRightChar() {
        if (cursorPos_ == (int)text_.size()) {
            return;
        }
        if ((cursorPos_ >= 0) && !text_.empty()) {
            text_.erase(cursorPos_, 1);
        }
    }

    void rightCursor() {
        if (text_.size() > 1) {
            if (cursorPos_ < int(text_.size())) {
                ++cursorPos_;
            }
        }
    }

    void leftCursor() {
        if (cursorPos_ > 0) {
            --cursorPos_;
        }
    }

    void homeCursor() {
        cursorPos_ = 0;
    }

    void endCursor() {
        cursorPos_ = text_.size();
    }

    void setCursorBarToggleTime(double time) { deltaTime_ = time; }
    void updateTime(double time) {
        double dt = time - prevTime_;
        if (dt >= deltaTime_) {
            toggleBar_ = !toggleBar_;
            prevTime_ = time;
        }
    }

    const std::string& text() const { return text_; }
    int cursorPos() const { return cursorPos_; }
    bool toggleCursorBar() const { return toggleBar_; }

protected:
    std::string text_;
    int cursorPos_ = 0;

    double deltaTime_ = 0.5;
    double prevTime_ = 0.0;
    bool toggleBar_ = true;

};

class TextEditFilter {
public:

    struct Regex {
        static const std::string Alphabet;          // "[a-xA-X]+"
        static const std::string UppercaseAlphabet; // "[A-X]+"
        static const std::string LowercaseAlphabet; // "[a-x]+"
        static const std::string Number;            // "\d+"
        static const std::string IntegerNumber;     // "[+-]?\d+"
        static const std::string HexIntegerNumber;  // "[+-]?(?:0[xX])?[0-9a-fA-F]+[hH]?)"
        static const std::string RealNumber;        // "[+-]?\d+(?:\.\d+)?"
    };

    static bool check(const std::string& text, const std::string& regexText);
};

class TextEdit {
public:
    TextEdit();
    virtual ~TextEdit();
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_TEXTEDIT_H_ */
