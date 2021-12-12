#ifndef PTGL_CORE_TEXTRENDERER_H_
#define PTGL_CORE_TEXTRENDERER_H_

#include <string>
#include <array>
#include <unordered_map>
#include "Common.h"

typedef struct NVGcontext NVGcontext;

namespace ptgl {

class GraphicsView;

class TextRenderer {
    friend class GraphicsView;
public:

    enum RenderState {
        RenderNoneState,
        RenderTextSceneState,
    };

    TextRenderer(GraphicsView* view);
    virtual ~TextRenderer();

    virtual void initializeConfiguration();

#if 0    // todo
    void loadFont(const std::string& path);
#endif

    void setTextColor(const std::array<double, 3>& rgb);
    void setTextColor(const std::array<double, 4>& rgba);
    void setTextColor(double r, double b, double g, double a = 1.0);
    void setTextAlign(Align align, VAlign valign);
    void setTextSize(int size);
    int textSize() const { return textSize_; }
    int textWidth() const;
    int textWidth(int n) const;
    int textHeight() const;

    void drawText(int x, int y, const std::string& text);
    void drawText(int x, int y, const std::string& text, const std::array<double, 4>& backgroundBoxColor);
    void drawText(int x, int y, const std::string& text, const std::array<double, 4>& backgroundBoxColor, int spacing);

    void drawText(double x, double y, double z, const std::string& text);
    void drawText(double x, double y, double z, const std::string& text, const std::array<double, 4>& backgroundBoxColor);
    void drawText(double x, double y, double z, const std::string& text, const std::array<double, 4>& backgroundBoxColor, int spacing);

    int windowWidth() const;
    int windowHeight() const;

    const GraphicsView* graphicsView() const { return graphicsView_; }

protected:

    float getTextOffset(int textSize);

    GraphicsView* graphicsView_ = nullptr;

    RenderState renderState_ = RenderNoneState;

    NVGcontext* nvgContext_ = nullptr;

    std::array<double, 4> textColor_{{1,1,1,1}};

    Align align_ = Align::Left;
    VAlign valign_ = VAlign::Bottom;

    std::string fontPath_;
    std::string fontName_;
    int textSize_;
    std::unordered_map<int, float> textOffsetCacheMap_;

protected:
    // for GraphicsView
    virtual void beginRender(RenderState state);
    virtual void endRender(RenderState state);
    void setRenderState(RenderState state) { renderState_ = state; }
};

} /* namespace ptgl */

#endif /* PTGL_CORE_TEXTRENDERER_H_ */
