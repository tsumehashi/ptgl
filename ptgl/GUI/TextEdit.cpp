#include "TextEdit.h"
#include <regex>

namespace ptgl {
namespace gui {

const std::string TextEditFilter::Regex::Alphabet          = R"([a-xA-X]+)";
const std::string TextEditFilter::Regex::UppercaseAlphabet = R"([A-X]+)";
const std::string TextEditFilter::Regex::LowercaseAlphabet = R"([a-x]+)";
const std::string TextEditFilter::Regex::Number            = R"(\d+)";
const std::string TextEditFilter::Regex::IntegerNumber     = R"([+-]?\d+)";
const std::string TextEditFilter::Regex::HexIntegerNumber  = R"([+-]?(?:0[xX])?[0-9a-fA-F]+[hH]?)";
const std::string TextEditFilter::Regex::RealNumber        = R"([+-]?\d+(?:\.\d+)?)";

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
