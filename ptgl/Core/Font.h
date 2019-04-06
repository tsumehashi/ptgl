#ifndef PTGL_CORE_FONT_H_
#define PTGL_CORE_FONT_H_

namespace ptgl {

class Font {
public:
    Font();
    virtual ~Font();

    static unsigned char* defaultFontData();
    static int defaultFontDataSize();

    // todo
    static int defaultFontTextHeight(int textSize);
    static int defaultFontTextWidth(int textSize);
    static int defaultFontTextWidth(int textSize, int n);
};

} /* namespace ptgl */

#endif /* PTGL_CORE_RESOURCE_FONTS_FONT_H_ */
