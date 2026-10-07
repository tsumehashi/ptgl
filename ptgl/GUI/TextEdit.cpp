#include "TextEdit.h"
#include <regex>

namespace ptgl {
namespace gui {


bool TextEditFilter::check(const std::string& text, const std::string& regexText)
{
    if (regexText.empty()) {
        // pass check
        return true;
    }
    std::regex re(regexText);
    return std::regex_match(text, re);
}

TextEdit::TextEdit() {

}

TextEdit::~TextEdit() {

}

}
} /* namespace ptgl */
