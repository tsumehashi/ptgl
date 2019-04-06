#include "Renderer2D.h"
#include <iostream>
#include "ptgl/Util/MathUtil.h"
#include "GraphicsView.h"
#include "PrimitiveShapeVertex.h"
#include "Renderer3D.h"
#include "DefaultShaderSource.h"
#include "Font.h"

#include "thirdparty/Core/nanovg/src/nanovg.h"

#define NANOVG_GLES2
#include "thirdparty/Core/nanovg/src/nanovg_gl.h"

namespace ptgl {

Renderer2D::Renderer2D(GraphicsView* view) : graphicsView_(view)
{
    rectAlignMode_ = Mode::Corner;
    ellipseAlignMode_ = Mode::Corner;
    fillColor_   = {{1.0, 1.0, 1.0, 1.0}};
    strokeColor_ = {{0.0, 0.0, 0.0, 1.0}};
    textColor_   = {{1.0, 1.0, 1.0, 1.0}};

    circleQuality_ = 40;

    align_ = Align::Left;
    valign_ = VAlign::Bottom;
    textSize_ = 20;
}

Renderer2D::~Renderer2D()
{
    nvgDeleteGLES2(renderNvgContext_);
    nvgDeleteGLES2(pickingNvgContext_);
}

void Renderer2D::initializeConfiguration()
{
    // set nanovg
    renderNvgContext_ = nvgCreateGLES2(NVG_ANTIALIAS);
    fontName_ = "defaultFont";
    nvgCreateFontMem(renderNvgContext_, fontName_.c_str(), Font::defaultFontData(), Font::defaultFontDataSize(), 0);

    // set picking nanovg
    pickingNvgContext_ = nvgCreateGLES2(0);    // don't use ANTIALIAS
}

int Renderer2D::windowWidth() const
{
    return graphicsView_->width();
}

int Renderer2D::windowHeight() const
{
    return graphicsView_->height();
}

// shader
void Renderer2D::setDefaultShaderProgram(ShaderProgramPtr shaderProgram)
{
    (void)shaderProgram;
    // don't use shader
}

void Renderer2D::restoreDefaultShaderProgram()
{
    // don't use shader
}

void Renderer2D::setShaderProgram(ShaderProgramPtr shaderProgram)
{
    (void)shaderProgram;
    // don't use shader
}

void Renderer2D::setForceUseShaderProgram(ShaderProgramPtr shaderProgram)
{
    (void)shaderProgram;
    // don't use shader
}

void Renderer2D::drawPoint(int x, int y)
{
    if (!isPickingRenderMode()) {
        const Eigen::Matrix4d& m = tf_.matrix();

        nvgResetTransform(nvgContext_);
        nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

        int s = pointSize_/2;
        if (s < 1) s = 1;
        nvgBeginPath(nvgContext_);
        nvgEllipse(nvgContext_, x, y, s, s);
        nvgFill(nvgContext_);
    }
#if 0
    Eigen::Affine3d affine = tf_.transformation() * Eigen::Translation3d(x, y, 0);

    auto& color = fillColor_;
    shaderProgram()->setParameter(unifPointSizeLocation_, pointSize_);
    shaderProgram()->setParameter(unifColorLocation_, color[0], color[1], color[2], color[3]);
    shaderProgram()->setParameter(unifModelMatrixLocation_, affine.matrix());
       Renderer3D::drawVertexBufferObject(primitivePointVBO_, attrVertexLocation_, GL_POINTS);
#endif
}

void Renderer2D::drawLine(int x1, int y1, int x2, int y2)
{
    const Eigen::Matrix4d& m = tf_.matrix();

    nvgResetTransform(nvgContext_);
    nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

    nvgBeginPath(nvgContext_);

    nvgMoveTo(nvgContext_, x1, y1);
    nvgLineTo(nvgContext_, x2, y2);

    nvgStroke(nvgContext_);
}

void Renderer2D::drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3)
{
    if (!fill_ && !stroke_) return;

    const Eigen::Matrix4d& m = tf_.matrix();

    nvgResetTransform(nvgContext_);
    nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

    if (fill_) {
        nvgBeginPath(nvgContext_);

        nvgMoveTo(nvgContext_, x1, y1);
        nvgLineTo(nvgContext_, x2, y2);
        nvgLineTo(nvgContext_, x3, y3);
        nvgClosePath(nvgContext_);

        nvgFill(nvgContext_);
    }

    if (stroke_) {
        nvgBeginPath(nvgContext_);

        nvgMoveTo(nvgContext_, x1, y1);
        nvgLineTo(nvgContext_, x2, y2);
        nvgLineTo(nvgContext_, x3, y3);
        nvgClosePath(nvgContext_);

        nvgStroke(nvgContext_);
    }
}

void Renderer2D::drawQuad(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
{
    if (!fill_ && !stroke_) return;

    const Eigen::Matrix4d& m = tf_.matrix();

    nvgResetTransform(nvgContext_);
    nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

    if (fill_) {
        nvgBeginPath(nvgContext_);

        nvgMoveTo(nvgContext_, x1, y1);
        nvgLineTo(nvgContext_, x2, y2);
        nvgLineTo(nvgContext_, x3, y3);
        nvgLineTo(nvgContext_, x4, y4);
        nvgClosePath(nvgContext_);

        nvgFill(nvgContext_);
    }

    if (stroke_) {
        nvgBeginPath(nvgContext_);

        nvgMoveTo(nvgContext_, x1, y1);
        nvgLineTo(nvgContext_, x2, y2);
        nvgLineTo(nvgContext_, x3, y3);
        nvgLineTo(nvgContext_, x4, y4);
        nvgClosePath(nvgContext_);

        nvgStroke(nvgContext_);
    }
}

void Renderer2D::drawRect(int x, int y, int width, int height)
{
    if (!fill_ && !stroke_) return;

    if (rectAlignMode_ == Mode::Center) {
        // center
        int w2 = width/2;
        int h2 = height/2;
        int x1 = x - w2;
        int y1 = y - h2;
        int x2 = x - w2;
        int y2 = y + h2;
        int x3 = x + w2;
        int y3 = y + h2;
        int x4 = x + w2;
        int y4 = y - h2;
        drawQuad(x1, y1, x2, y2, x3, y3, x4, y4);
    } else {
        // corner
        int x1 = x;
        int y1 = y;
        int x2 = x;
        int y2 = y + height;
        int x3 = x + width;
        int y3 = y + height;
        int x4 = x + width;
        int y4 = y;
        drawQuad(x1, y1, x2, y2, x3, y3, x4, y4);
    }
}

void Renderer2D::drawEllipse(int x, int y, int width, int height)
{
    if (!fill_ && !stroke_) return;

    int ox = 0;    // offset x
    int oy = 0; // offset y
    if (ellipseAlignMode_ == Mode::Corner) {
        // corner
        ox = width/2;
        oy = height/2;
    }

    const Eigen::Matrix4d& m = tf_.matrix();

    nvgResetTransform(nvgContext_);
    nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

    if (fill_) {
        nvgBeginPath(nvgContext_);
        nvgEllipse(nvgContext_, x-ox, y-oy, width/2, height/2);
        nvgFill(nvgContext_);
    }

    if (stroke_) {
        nvgBeginPath(nvgContext_);
        nvgEllipse(nvgContext_, x-ox, y-oy, width/2, height/2);
        nvgStroke(nvgContext_);
    }
}

void Renderer2D::drawCircle(int x, int y, int radius)
{
    drawEllipse(x, y, 2*radius, 2*radius);
}

void Renderer2D::drawRing(int x, int y, int outer_radius, int inner_radius)
{
    if (!fill_ && !stroke_) return;

    // todo
    (void)x;
    (void)y;
    (void)outer_radius;
    (void)inner_radius;
}

void Renderer2D::drawBezier(int x, int y, int cx1, int cy1, int cx2, int cy2, int x2, int y2)
{
    if (!fill_ && !stroke_) return;

    // todo
    (void)x;
    (void)y;
    (void)cx1;
    (void)cy1;
    (void)cx2;
    (void)cy2;
    (void)x2;
    (void)y2;
}

void Renderer2D::drawPoint(Point2i* points, int size)
{
    if (!isPickingRenderMode()) {
        const Eigen::Matrix4d& m = tf_.matrix();

        nvgResetTransform(nvgContext_);
        nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

        int s = pointSize_/2;
        if (s < 1) s = 1;
        nvgBeginPath(nvgContext_);
        for (int i = 0; i < size; ++i) {
            Point2i* p = &points[i];
            nvgEllipse(nvgContext_, p->x, p->y, s, s);
        }
        nvgFill(nvgContext_);
    }
}

void Renderer2D::drawLines(Point2i* points, int size)
{
    if (size < 2) {
        return;
    }

    const Eigen::Matrix4d& m = tf_.matrix();

    nvgResetTransform(nvgContext_);
    nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

    nvgBeginPath(nvgContext_);

    nvgMoveTo(nvgContext_, points[0].x, points[0].y);
    for (int i = 1; i < size; ++i) {
        Point2i* p = &points[i];
        nvgLineTo(nvgContext_, p->x, p->y);
    }

    nvgStroke(nvgContext_);
}

void Renderer2D::drawVertex(const VertexList& vertices, const IndexList& indices)
{
    if (!fill_ && !stroke_) return;
    if (vertices.empty()) return;

    const Eigen::Matrix4d& m = tf_.matrix();

    nvgResetTransform(nvgContext_);
    nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

    if (fill_) {
        nvgBeginPath(nvgContext_);

        const auto& vertex0 = vertices[0];
        nvgMoveTo(nvgContext_, vertex0.x, vertex0.y);

        for (size_t i = 1; i < indices.size(); ++i) {
            size_t id = indices[i];
            const auto& vertex = vertices[id];
            nvgLineTo(nvgContext_, vertex.x, vertex.y);
        }
        nvgClosePath(nvgContext_);

        nvgFill(nvgContext_);
    }

    if (stroke_) {
        nvgBeginPath(nvgContext_);

        const auto& vertex0 = vertices[0];
        nvgMoveTo(nvgContext_, vertex0.x, vertex0.y);

        for (size_t i = 1; i < indices.size(); ++i) {
            size_t id = indices[i];
            const auto& vertex = vertices[id];
            nvgLineTo(nvgContext_, vertex.x, vertex.y);
        }
        nvgClosePath(nvgContext_);

        nvgStroke(nvgContext_);
    }
}

void Renderer2D::setRectMode(Mode mode)
{
    rectAlignMode_ = mode;
}

void Renderer2D::setEllipseMode(Mode mode)
{
    ellipseAlignMode_ = mode;
}

void Renderer2D::setFillColor(const std::array<double, 3>& rgb)
{
    setFillColor(rgb[0], rgb[1], rgb[2]);
}

void Renderer2D::setFillColor(const std::array<double, 4>& rgba)
{
    setFillColor(rgba[0], rgba[1], rgba[2], rgba[3]);
}

void Renderer2D::setFillColor(double r, double g, double b, double a)
{
    fillColor_ = {{r,g,b,a}};
    fill_ = true;

    if (!isPickingRenderMode()) {
        nvgFillColor(nvgContext_, NVGcolor{(float)fillColor_[0], (float)fillColor_[1], (float)fillColor_[2], (float)fillColor_[3]});
    }
}

void Renderer2D::setNoFill()
{
    fill_ = false;
}

void Renderer2D::setStrokeColor(const std::array<double, 3>& rgb)
{
    setStrokeColor(rgb[0], rgb[1], rgb[2]);
}

void Renderer2D::setStrokeColor(const std::array<double, 4>& rgba)
{
    setStrokeColor(rgba[0], rgba[1], rgba[2], rgba[3]);
}

void Renderer2D::setStrokeColor(double r, double g, double b, double a)
{
    strokeColor_ = {{r,g,b,a}};
    stroke_ = true;

    if (!isPickingRenderMode()) {
        nvgStrokeColor(nvgContext_, NVGcolor{(float)strokeColor_[0], (float)strokeColor_[1], (float)strokeColor_[2], (float)strokeColor_[3]});
    }
}

void Renderer2D::setNoStroke()
{
    stroke_ = false;
}

void Renderer2D::setStrokeWeight(int w)
{
    strokeWeight_ = w;
    nvgStrokeWidth(nvgContext_, strokeWeight_);
}

void Renderer2D::setPointSize(int s)
{
    pointSize_ = s;
}

// Text
void Renderer2D::setTextAlign(Align align, VAlign vAlign)
{
    align_ = align;
    valign_ = vAlign;
}

void Renderer2D::setTextSize(int size)
{
    textSize_ = size;
}

void Renderer2D::setTextColor(const std::array<double, 3>& rgb)
{
    setTextColor(rgb[0], rgb[1], rgb[2]);
//    if (!isPickingRenderMode()) {
//        nvgFillColor(nvgContext_, NVGcolor{(float)textColor_[0], (float)textColor_[1], (float)textColor_[2], (float)textColor_[3]});
//    }
}

void Renderer2D::setTextColor(const std::array<double, 4>& rgba)
{
    textColor_ = rgba;
//    if (!isPickingRenderMode()) {
//        nvgFillColor(nvgContext_, NVGcolor{(float)textColor_[0], (float)textColor_[1], (float)textColor_[2], (float)textColor_[3]});
//    }
}

void Renderer2D::setTextColor(double r, double g, double b, double a)
{
    textColor_ = {{r,g,b,a}};
//    if (!isPickingRenderMode()) {
//        nvgFillColor(nvgContext_, NVGcolor{(float)textColor_[0], (float)textColor_[1], (float)textColor_[2], (float)textColor_[3]});
//    }
}

int Renderer2D::textSize() const
{
    return textSize_;
}

int Renderer2D::textWidth() const
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

int Renderer2D::textWidth(int n) const
{
    return textWidth() * n;
}

int Renderer2D::textHeight() const
{
    return textSize();
}

float Renderer2D::getTextOffset(int textSize)
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

void Renderer2D::drawText(int x, int y, const std::string& str)
{
    if (isPickingRenderMode()) return;

    Eigen::Affine3d affine = tf_.transformation() * Eigen::Translation3d(x, y, 0);
    Eigen::Vector3d p = affine.translation();
    Eigen::Matrix3d m = affine.linear();
    // todo rotate
    double sx = sqrt(m(0,0) * m(0,0) + m(1,0) * m(1,0));
    double sy = sqrt(m(0,1) * m(0,1) + m(1,1) * m(1,1));
    double textScale = std::min(sx, sy);
    double textSize = textScale * textSize_;

    int yoffset = getTextOffset(textSize);

    nvgBeginPath(nvgContext_);
    nvgFontFace(nvgContext_, fontName_.c_str());

    nvgFillColor(nvgContext_, NVGcolor{(float)textColor_[0], (float)textColor_[1], (float)textColor_[2], (float)textColor_[3]});
    nvgFontSize(nvgContext_, textSize);
    nvgText(nvgContext_, p(0), p(1) - yoffset, str.c_str(), NULL);

    // restore FillColor
    nvgFillColor(nvgContext_, NVGcolor{(float)fillColor_[0], (float)fillColor_[1], (float)fillColor_[2], (float)fillColor_[3]});
}

// Image
void Renderer2D::registerImage(const std::string& name, ImagePtr image, bool override)
{
    if (!image) return;
    if (image->pixelType() != Image::PixelType::RGBA) {
        // todo
        std::cerr << "Renderer2D::registerImage not RGBA" << std::endl;
        return;
    }

    registerImage(name, image->width(), image->height(), image->data(), override);
}

void Renderer2D::registerImage(const std::string& name, int width, int height, const uint8_t* rgbaData, bool override)
{
    bool createImage = false;

    auto fitr = registeredImageInfoMap_.find(name);
    if (fitr != registeredImageInfoMap_.end()) {
        // find
        if (override) {
            auto& regImageInfo = fitr->second;
            if ((regImageInfo.width == width)
              &&(regImageInfo.height == height)) {
                // same size
                // update
                nvgUpdateImage(nvgContext(), regImageInfo.nvgImage, rgbaData);
            } else {
                // delete
                nvgDeleteImage(nvgContext(), regImageInfo.nvgImage);

                // create
                createImage = true;
            }
        }
    } else {
        // not find
        createImage = true;
    }

    if (createImage) {
        ImageInfo info;
        info.name = name;
        info.nvgImage = nvgCreateImageRGBA(nvgContext(), width, height, 0, rgbaData);
        info.width = width;
        info.height = height;
        registeredImageInfoMap_[name] = info;    // override
    }
}

void Renderer2D::updateRegisteredImage(const std::string& name, ImagePtr image)
{
    if (!image) return;
    if (image->pixelType() != Image::PixelType::RGBA) {
        // todo
        std::cerr << "Renderer2D::updateRegisteredImage not RGBA" << std::endl;
        return;
    }

    updateRegisteredImage(name, image->width(), image->height(), image->data());
}

bool Renderer2D::isRegisterdImage(const std::string& name)
{
    auto fitr = registeredImageInfoMap_.find(name);
    return fitr != registeredImageInfoMap_.end();
}

void Renderer2D::updateRegisteredImage(const std::string& name, int width, int height, const uint8_t* rgbaData)
{
    auto fitr = registeredImageInfoMap_.find(name);
    if (fitr != registeredImageInfoMap_.end()) {
        // find
        auto& regImageInfo = fitr->second;
        if ((regImageInfo.width == width)
          &&(regImageInfo.height == height)) {
            // same size
            // update
            nvgUpdateImage(nvgContext(), regImageInfo.nvgImage, rgbaData);
        } else {
            std::cerr << "Renderer2D::updateRegisteredImage not match width and height" << std::endl;
        }
    }
}

void Renderer2D::drawRegisteredImage(const std::string& name, int x, int y)
{
    auto fitr = registeredImageInfoMap_.find(name);
    if (fitr == registeredImageInfoMap_.end()) {
        return;
    }
    auto& imageInfo = fitr->second;

    const Eigen::Matrix4d& m = tf_.matrix();

    nvgResetTransform(nvgContext_);
    nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));


    NVGpaint pattern = nvgImagePattern(nvgContext(), x, y, imageInfo.width, imageInfo.height, 0, imageInfo.nvgImage, 1);

    nvgBeginPath(nvgContext());
    nvgRect(nvgContext(), x, y, imageInfo.width, imageInfo.height);
    nvgFillPaint(nvgContext(), pattern);
    nvgFill(nvgContext());
}

void Renderer2D::drawRegisteredImage(const std::string& name, int x, int y, int w, int h)
{
    auto fitr = registeredImageInfoMap_.find(name);
    if (fitr == registeredImageInfoMap_.end()) {
        return;
    }
    auto& imageInfo = fitr->second;

#if 0
    double sx = double(w) / imageInfo.width;
    double sy = double(h) / imageInfo.height;

    const Eigen::Matrix4d& m = tf_.matrix();

    nvgResetTransform(nvgContext_);
    nvgScale(nvgContext_, sx, sy);
    nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

    NVGpaint pattern = nvgImagePattern(nvgContext(), x, y, imageInfo.width, imageInfo.height, 0, imageInfo.nvgImage, 1);

    nvgBeginPath(nvgContext());
    nvgRect(nvgContext(), x, y, imageInfo.width, imageInfo.height);
    nvgFillPaint(nvgContext(), pattern);
    nvgFill(nvgContext());
#else

    const Eigen::Matrix4d& m = tf_.matrix();

    nvgResetTransform(nvgContext_);
    nvgTransform(nvgContext_, m(0,0), m(1,0), m(0,1), m(1,1), m(0,3), m(1,3));

    NVGpaint pattern = nvgImagePattern(nvgContext(), x, y, w, h, 0, imageInfo.nvgImage, 1);

    nvgBeginPath(nvgContext());
    nvgRect(nvgContext(), x, y, w, h);
    nvgFillPaint(nvgContext(), pattern);
    nvgFill(nvgContext());
#endif
}

void Renderer2D::pushMatrix()
{
    tf_.push();
}

void Renderer2D::popMatrix()
{
    tf_.pop();
}

void Renderer2D::translate(int x, int y)
{
    tf_.translate(x, y, 0);
}

void Renderer2D::rotate(double degree)
{
    tf_.rotate(ptgl::radian(degree), Eigen::Vector3d::UnitZ());
}

void Renderer2D::scale(double scale)
{
    tf_.scale(scale, scale, 1.0);
}

void Renderer2D::scale(double scaleX, double scaleY)
{
    tf_.scale(scaleX, scaleY, 1.0);
}

const Eigen::Affine3d& Renderer2D::transformation() const
{
    return tf_.transformation();
}

void Renderer2D::beginScissor(int x, int y, int w, int h)
{
    nvgScissor(nvgContext_, x, y, w, h);
}

void Renderer2D::endScissor()
{
    nvgResetScissor(nvgContext_);
}

bool Renderer2D::isPickingMode() const
{
    return renderState_ == RenderPickingState;
}

void Renderer2D::setPickColor(const std::array<double, 4>& color)
{
    pickColor_ = color;

    if (isPickingRenderMode()) {
        nvgFillColor(pickingNvgContext_, NVGcolor{(float)pickColor_[0], (float)pickColor_[1], (float)pickColor_[2], (float)pickColor_[3]});
        nvgStrokeColor(pickingNvgContext_, NVGcolor{(float)pickColor_[0], (float)pickColor_[1], (float)pickColor_[2], (float)pickColor_[3]});
    }
}

// for GraphicsView
void Renderer2D::beginRender(RenderState state)
{
    setRenderState(state);

    tf_.clear();

    if (!isPickingRenderMode()) {
        nvgContext_ = renderNvgContext_;
    } else {
        nvgContext_ = pickingNvgContext_;
    }

    nvgBeginFrame(nvgContext_, windowWidth(), windowHeight(), pixelRatio_);
}

void Renderer2D::endRender(RenderState state)
{
    (void)state;

    nvgEndFrame(nvgContext_);

    // todo
    setRenderState(RenderNoneState);
}

} /* namespace ptgl */
