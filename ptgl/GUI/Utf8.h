#ifndef PTGL_GUI_UTF8_H_
#define PTGL_GUI_UTF8_H_
#include <string>
#include <cstdint>
namespace ptgl::gui::utf8
{
inline std::string encode(std::uint32_t c)
{
    std::string s;
    if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff))
        return s;
    if (c < 0x80)
        s += char(c);
    else if (c < 0x800) {
        s += char(0xc0 | (c >> 6));
        s += char(0x80 | (c & 63));
    } else if (c < 0x10000) {
        s += char(0xe0 | (c >> 12));
        s += char(0x80 | ((c >> 6) & 63));
        s += char(0x80 | (c & 63));
    } else {
        s += char(0xf0 | (c >> 18));
        s += char(0x80 | ((c >> 12) & 63));
        s += char(0x80 | ((c >> 6) & 63));
        s += char(0x80 | (c & 63));
    }
    return s;
}
inline size_t next(const std::string &s, size_t p)
{
    if (p < s.size())
        ++p;
    while (p < s.size() && (static_cast<unsigned char>(s[p]) & 0xc0) == 0x80)
        ++p;
    return p;
}
inline size_t previous(const std::string &s, size_t p)
{
    if (p > 0)
        --p;
    while (p > 0 && (static_cast<unsigned char>(s[p]) & 0xc0) == 0x80)
        --p;
    return p;
}
inline bool valid(const std::string &s)
{
    for (size_t p = 0; p < s.size();) {
        auto b = static_cast<unsigned char>(s[p++]);
        uint32_t c = b;
        int n = 0;
        if (b >= 0xf0 && b <= 0xf4) {
            c = b & 7;
            n = 3;
        } else if (b >= 0xe0 && b <= 0xef) {
            c = b & 15;
            n = 2;
        } else if (b >= 0xc2 && b <= 0xdf) {
            c = b & 31;
            n = 1;
        } else if (b >= 0x80)
            return false;
        int count = n;
        while (n--) {
            if (p >= s.size())
                return false;
            auto t = static_cast<unsigned char>(s[p++]);
            if ((t & 0xc0) != 0x80)
                return false;
            c = (c << 6) | (t & 63);
        }
        if (c > 0x10ffff || (c >= 0xd800 && c <= 0xdfff) || (count == 2 && c < 0x800) ||
            (count == 3 && c < 0x10000))
            return false;
    }
    return true;
}
} // namespace ptgl::gui::utf8
#endif
