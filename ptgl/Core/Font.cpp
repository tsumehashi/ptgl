#include "Font.h"

// load default font
namespace {
#include "thirdparty/Fonts/RobotoMono_Regular.h"
}

namespace ptgl {

// resource_fonts_RobotoMono_Regular_ttf
static const int font_width_map_RobotoMono_Regular_ttf[] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7,
    7, 8, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14,
    15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 20, 21, 21,
    22, 22, 23, 23, 24, 24, 25, 25, 25, 26, 26, 27, 27, 28, 28, 29,
    29, 30, 30, 30, 31, 31, 32, 32, 33, 33, 34, 34, 35, 35, 35, 36,
    36, 37, 37, 38, 38, 39, 39, 40, 40, 40, 41, 41, 42, 42, 43, 43,
    44, 44, 45, 45, 46, 46, 46, 47, 47, 48, 48, 49, 49, 50, 50, 51,
    51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 56, 57, 57, 58,
    58, 59, 59, 60, 60, 61, 61, 61, 62, 62, 63, 63, 64, 64, 65, 65,
    66, 66, 66, 67, 67, 68, 68, 69
};
static const int font_width_map_RobotoMono_Regular_ttf_size = sizeof(font_width_map_RobotoMono_Regular_ttf) / sizeof(font_width_map_RobotoMono_Regular_ttf[0]);

Font::Font() {

}

Font::~Font() {

}

unsigned char* Font::defaultFontData()
{
    return &resource_fonts_RobotoMono_Regular_ttf[0];
}

int Font::defaultFontDataSize()
{
    return resource_fonts_RobotoMono_Regular_ttf_len;
}

int Font::defaultFontTextHeight(int textSize)
{
    return textSize;
}

int Font::defaultFontTextWidth(int textSize)
{
    if (textSize < font_width_map_RobotoMono_Regular_ttf_size) {
        return font_width_map_RobotoMono_Regular_ttf[textSize];
    } else {
        return font_width_map_RobotoMono_Regular_ttf[textSize-1];
    }
}

int Font::defaultFontTextWidth(int textSize, int n)
{
    return defaultFontTextWidth(textSize) * n;
}

} /* namespace ptgl */
