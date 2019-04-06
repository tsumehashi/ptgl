#include "TextRenderer.h"
#include "GraphicsView.h"
#include "Font.h"

#include "thirdparty/Core/nanovg/src/nanovg.h"

#define NANOVG_GLES2
#include "thirdparty/Core/nanovg/src/nanovg_gl.h"

namespace ptgl {

TextRenderer::TextRenderer(GraphicsView* view)
    : graphicsView_(view)
{
    textSize_ = 20;

    // set default font path
//    fontPath_ = "resource/fonts/roboto_mono/RobotoMono-Regular.ttf";
//    fontName_ = "RobotoMono-Regular";
}

TextRenderer::~TextRenderer()
{
    nvgDeleteGLES2(nvgContext_);
}

void TextRenderer::initializeConfiguration()
{
    nvgContext_ = nvgCreateGLES2(NVG_ANTIALIAS);

    // load font
//    nvgCreateFont(nvgContext_, fontName_.c_str(), fontPath_.c_str());

    fontName_ = "defaultFont";
    nvgCreateFontMem(nvgContext_, fontName_.c_str(), Font::defaultFontData(), Font::defaultFontDataSize(), 0);
}

#if 0
void TextRenderer::loadFont(const std::string& path)
{

}
#endif

void TextRenderer::setTextColor(const std::array<double, 3>& rgb)
{
    setTextColor(rgb[0], rgb[1], rgb[2]);
}

void TextRenderer::setTextColor(const std::array<double, 4>& rgba)
{
    setTextColor(rgba[0], rgba[1], rgba[2], rgba[3]);
}

void TextRenderer::setTextColor(double r, double b, double g, double a)
{
    textColor_ = {{r,g,b,a}};
    nvgFillColor(nvgContext_, NVGcolor{(float)textColor_[0], (float)textColor_[1], (float)textColor_[2], (float)textColor_[3]});
}

void TextRenderer::setTextAlign(Align align, VAlign valign)
{
    align_ = align;
    valign_ = valign;
}

void TextRenderer::setTextSize(int size)
{
    textSize_ = size;
}

int TextRenderer::textWidth() const
{
#if 0
    auto fitr = textWidthCacheMap_.find(textSize());
    if (fitr != textWidthCacheMap_.end()) {
        // find
        return fitr->second;
    }

    nvgBeginPath(nvgContext_);
    nvgFontFace(nvgContext_, fontName_.c_str());
    nvgFontSize(nvgContext_, textSize());
    const char* aaa = "a";
    float w = nvgTextBounds(nvgContext_, 0, 0, aaa, nullptr, nullptr);
    textWidthCacheMap_.insert({textSize(), w});
    return w;
#else
    return Font::defaultFontTextWidth(textSize());
#endif
}

int TextRenderer::textWidth(int n) const
{
    return textWidth() * n;
}

int TextRenderer::textHeight() const
{
    return textSize();
}

float TextRenderer::getTextOffset(int textSize)
{
    auto fitr = textOffsetCacheMap_.find(textSize);
    if (fitr != textOffsetCacheMap_.end()) {
        // find
        return fitr->second;
    }

    nvgBeginPath(nvgContext_);
    nvgFontFace(nvgContext_, fontName_.c_str());
    nvgFontSize(nvgContext_, textSize);
    const char* aaa = "a";
    float b[4];
    nvgTextBounds(nvgContext_, 0, 0, aaa, nullptr, b);
    textOffsetCacheMap_.insert({textSize, b[3]});
    return b[3];
}

void TextRenderer::drawText(int x, int y, const std::string& text)
{
    int yo = getTextOffset(textSize());
    nvgFontSize(nvgContext_, textSize());
//    nvgFillColor(nvgContext_, NVGcolor{textColor_[0], textColor_[1], textColor_[2], textColor_[3]});
    nvgText(nvgContext_, x, y - yo, text.c_str(), NULL);
}

void TextRenderer::drawText(int x, int y, const std::string& text, const std::array<double, 4>& backgroundBoxColor)
{
    int w = textWidth(text.size());
    int s = w * 0.05;    // spacing
    if (s < 3) s = 3;
    int spacing = s;
    drawText(x, y, text, backgroundBoxColor, spacing);
}

void TextRenderer::drawText(int x, int y, const std::string& text, const std::array<double, 4>& backgroundBoxColor, int spacing)
{
    int w = textWidth(text.size());
    int h = textHeight();
    int s = spacing;    // spacing
    if (s < 0) s = 0;
    int x1 = x - s;
    int y1 = y - h - s;
    int x2 = x + w + s;
    int y2 = y - h - s;
    int x3 = x + w + s;
    int y3 = y + s;
    int x4 = x - s;
    int y4 = y + s;

    // draw background box
    nvgBeginPath(nvgContext_);

    nvgFillColor(nvgContext_, NVGcolor{(float)backgroundBoxColor[0], (float)backgroundBoxColor[1], (float)backgroundBoxColor[2], (float)backgroundBoxColor[3]});

    nvgMoveTo(nvgContext_, x1, y1);
    nvgLineTo(nvgContext_, x2, y2);
    nvgLineTo(nvgContext_, x3, y3);
    nvgLineTo(nvgContext_, x4, y4);
    nvgClosePath(nvgContext_);

    nvgFill(nvgContext_);

    // draw text
    nvgFillColor(nvgContext_, NVGcolor{(float)textColor_[0], (float)textColor_[1], (float)textColor_[2], (float)textColor_[3]});
    drawText(x, y, text);
}

#if 0    // todo
void TextRenderer::drawText(double x, double y, double z, const std::string& text)
{

}
#endif

int TextRenderer::windowWidth() const
{
    return graphicsView_->width();
}

int TextRenderer::windowHeight() const
{
    return graphicsView_->height();
}

void TextRenderer::beginRender(RenderState state)
{
    setRenderState(state);

    int width = windowWidth();
    int height = windowHeight();
    float pixelRatio = 1.0;

    nvgBeginFrame(nvgContext_, width, height, pixelRatio);

    nvgBeginPath(nvgContext_);

    nvgFontFace(nvgContext_, fontName_.c_str());
}

void TextRenderer::endRender(RenderState state)
{
    (void)state;
    nvgEndFrame(nvgContext_);

    setRenderState(RenderNoneState);
}

} /* namespace ptgl */
