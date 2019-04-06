#ifndef PTGL_CORE_RENDERER2D_H_
#define PTGL_CORE_RENDERER2D_H_

#include <array>
#include <memory>
#include <vector>
#include <unordered_map>
#include "Common.h"
#include "ShaderProgram.h"
#include "TransformStack.h"
#include "VertexBufferObject.h"
#include "Image.h"

typedef struct NVGcontext NVGcontext;

namespace ptgl {

class GraphicsView;

class Renderer2D {
    friend class GraphicsView;
public:

    enum RenderState {
        RenderNoneState,
        RenderBackgroundSceneState,
        RenderSceneState,
        RenderPickingState,
    };

    enum class Mode {
        Center,
        Corner,
    };

    using Align = ::ptgl::Align;
    using VAlign = ::ptgl::VAlign;

    Renderer2D(GraphicsView* view);
    virtual ~Renderer2D();

    virtual void initializeConfiguration();

    RenderState renderState() const { return renderState_; }

    // camera
    int windowWidth() const;
    int windowHeight() const;

    // shader
    void setDefaultShaderProgram(ShaderProgramPtr shaderProgram);
    void restoreDefaultShaderProgram();
    void setShaderProgram(ShaderProgramPtr shaderProgram);
    ShaderProgramPtr shaderProgram() { return nullptr; }

    void setForceUseShaderProgram(ShaderProgramPtr shaderProgram);

    // Primitive shapes
    void drawPoint(int x, int y);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3);
    void drawQuad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
    void drawRect(int x, int y, int width, int height);
    void drawEllipse(int x, int y, int width, int height);
    void drawCircle(int x, int y, int radius);
    void drawRing(int x, int y, int outer_radius, int inner_radius);

    void drawBezier(int x, int y, int cx1, int cy1, int cx2, int cy2, int x2, int y2);

    struct Point2i {
        Point2i() : x(0), y(0) {}
        Point2i(int x, int y) : x(x), y(y) {}
        int x;
        int y;
    };
    void drawPoint(Point2i* points, int size);
    void drawLines(Point2i* points, int size);

    // draw vertex
    void drawVertex(const VertexList& vertices, const IndexList& indices);

    void setRectMode(Mode mode);
    Mode rectMode() const { return rectAlignMode_; }

    void setEllipseMode(Mode mode);
    Mode ellipseMode() const { return ellipseAlignMode_; }

    const std::array<double, 4>& fillColor() const { return fillColor_; }
    void setFillColor(const std::array<double, 3>& rgb);
    void setFillColor(const std::array<double, 4>& rgba);
    void setFillColor(double r, double g, double b, double a = 1.0);
    void setNoFill();

    const std::array<double, 4>& strokeColor() const { return strokeColor_; }
    void setStrokeColor(const std::array<double, 3>& rgb);
    void setStrokeColor(const std::array<double, 4>& rgba);
    void setStrokeColor(double r, double g, double b, double a = 1.0);
    void setNoStroke();

    void setStrokeWeight(int w);
    void setPointSize(int s);

    void setCircleQuality(int q) { circleQuality_ = q; }
    int circleQuality() const { return circleQuality_; }

    // Text
    void setTextAlign(Align align, VAlign vAlign);
    void setTextSize(int size);
    void setTextColor(const std::array<double, 3>& rgb);
    void setTextColor(const std::array<double, 4>& rgba);
    void setTextColor(double r, double g, double b, double a = 1.0);
    int textSize() const;
    int textWidth() const;
    int textWidth(int n) const;
    int textHeight() const;
#if 0
    void loadFont(const std::string& path);
#endif
    void drawText(int x, int y, const std::string& str);


    // Image
    void registerImage(const std::string& name, ImagePtr image, bool overwrite = true);
    void registerImage(const std::string& name, int width, int height, const uint8_t* rgbaData, bool overwrite = true);
    void updateRegisteredImage(const std::string& name, ImagePtr image);
    void updateRegisteredImage(const std::string& name, int width, int height, const uint8_t* rgbaData);
    bool isRegisterdImage(const std::string& name);
    void drawRegisteredImage(const std::string& name, int x, int y);
    void drawRegisteredImage(const std::string& name, int x, int y, int w, int h);

    // transforms
    void pushMatrix();
    void popMatrix();

    void translate(int x, int y);
    void rotate(double degree);
    void scale(double scale);
    void scale(double scaleX, double scaleY);

    const TransformStack& transformStack() const { return tf_; }
    const Eigen::Affine3d& transformation() const;

    // scissor
    void beginScissor(int x, int y, int w, int h);
    void endScissor();

    // nanovg
    NVGcontext* nvgContext() const { return nvgContext_; }
    NVGcontext* pickingNvgContext() const { return pickingNvgContext_; }

    // picking mode
    bool isPickingMode() const;
    void setPickColor(const std::array<double, 4>& color);
    const std::array<double, 4>& pickColor() const { return pickColor_; }

    const GraphicsView* graphicsView() const { return graphicsView_; }

protected:

    bool isPickingRenderMode() {
        return renderState_ == RenderPickingState;
    }

    float getTextOffset(int textSize);

    GraphicsView* graphicsView_ = nullptr;

    RenderState renderState_ = RenderNoneState;

    NVGcontext* nvgContext_ = nullptr;
    NVGcontext* renderNvgContext_ = nullptr;
    NVGcontext* pickingNvgContext_ = nullptr;
    float pixelRatio_ = 1.0f;

    std::array<double, 4> fillColor_;
    std::array<double, 4> strokeColor_;
    bool fill_ = true;
    bool stroke_ = true;

    int circleQuality_;

    Mode rectAlignMode_ = Mode::Center;
    Mode ellipseAlignMode_ = Mode::Center;

    TransformStack tf_;

    // text
    Align align_ = Align::Left;
    VAlign valign_ = VAlign::Bottom;

    std::array<double, 4> textColor_;

    std::string fontPath_;
    std::string fontName_;
    int textSize_;
    std::unordered_map<int, float> textOffsetCacheMap_;

    // image
    struct ImageInfo {
        std::string name;
        int nvgImage = -1;
        int width = 0;
        int height = 0;
        ImagePtr image;
    };
    std::unordered_map<std::string, ImageInfo> registeredImageInfoMap_;

    int strokeWeight_ = 1;
    int pointSize_ = 1;

    // picking mode
    std::array<double, 4> pickColor_;

protected:
    // for GraphicsView
    virtual void beginRender(RenderState state);
    virtual void endRender(RenderState state);
    void setRenderState(RenderState state) { renderState_ = state; }

};

} /* namespace ptgl */

#endif /* PTGL_CORE_RENDERER2D_H_ */
