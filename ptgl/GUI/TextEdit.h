#ifndef PTGL_GUI_TEXTEDIT_H_
#define PTGL_GUI_TEXTEDIT_H_

#include <string>
#include "TextBuffer.h"

namespace ptgl {
namespace gui {

class TextEditFilter {
public:

    struct Regex {
        inline static const std::string Alphabet = R"([a-zA-Z]+)";       // "[a-xA-X]+"
        inline static const std::string UppercaseAlphabet = R"([A-Z]+)"; // "[A-X]+"
        inline static const std::string LowercaseAlphabet = R"([a-z]+)"; // "[a-x]+"
        inline static const std::string Number = R"(\d+)";               // "\d+"
        inline static const std::string IntegerNumber = R"([+-]?\d+)";   // "[+-]?\d+"
        inline static const std::string HexIntegerNumber =
            R"([+-]?(?:0[xX])?[0-9a-fA-F]+[hH]?)"; // "[+-]?(?:0[xX])?[0-9a-fA-F]+[hH]?)"
        inline static const std::string RealNumber = R"([+-]?\d+(?:\.\d+)?)"; // "[+-]?\d+(?:\.\d+)?"
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
