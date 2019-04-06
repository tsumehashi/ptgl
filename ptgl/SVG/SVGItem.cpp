#include "SVGItem.h"
#include <set>
#include <list>
#include <random>
#include <iostream>
#include "ptgl/Core/Renderer2D.h"

#include "thirdparty/Core/nanovg/src/nanovg.h"
#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include "thirdparty/SVG/nanosvg/src/nanosvg.h"

namespace ptgl {
namespace svg {

namespace {

std::array<double, 4> getColor(unsigned int c) {
    double a = (unsigned char)((c & 0xff000000) >> 24);
    double b = (unsigned char)((c & 0x00ff0000) >> 16);
    double g = (unsigned char)((c & 0x0000ff00) >> 8);
    double r = (unsigned char)((c & 0x000000ff) >> 0);
    return {r/255.f,g/255.f,b/255.f,a/255.f};
};

SVGShapeList loadSVGSub(NSVGimage* image) {
    if (!image) {
        return SVGShapeList();
    }

    SVGShapeList svgShapeList;
    int index = 0;
    for (NSVGshape* shape = image->shapes; shape != NULL; shape = shape->next) {

        if (!(shape->flags & NSVG_FLAGS_VISIBLE))
            continue;

        SVGShapePtr svgShape = std::make_shared<SVGShape>();
        svgShape->name = shape->id;
        svgShape->index = index;
        svgShape->fillColor = getColor(shape->fill.color);
        svgShape->strokeColor = getColor(shape->stroke.color);
        svgShape->strokeWidth = shape->strokeWidth;
        svgShape->stroke = shape->stroke.type;
        svgShape->fill = shape->fill.type;
        svgShape->bounds = std::array<double, 4>{shape->bounds[0], shape->bounds[1], shape->bounds[2], shape->bounds[3]};

        for (NSVGpath* path = shape->paths; path != NULL; path = path->next) {

            SVGPath svgPath;
            svgPath.closed = path->closed;
            for (int i = 0; i < path->npts*2; ++i) {
                svgPath.pts.push_back(path->pts[i]);
            }
            svgShape->pathList.push_back(svgPath);
        }

        svgShapeList.push_back(svgShape);

        ++index;
    }

    return svgShapeList;
}

}    // namespace

SVGItem::SVGItem()
{

}

SVGItem::SVGItem(const SVGShapeList& s)
{
    setSVGShapes(s);
}

SVGItem::~SVGItem()
{

}

void SVGItem::setSVGShapes(const SVGShapeList& s)
{
    shapes = s;

    bounds = {std::numeric_limits<float>::max(),std::numeric_limits<float>::max(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    for (auto& shape : shapes) {
        auto& b = shape->bounds;
        if (bounds[0] > b[0]) bounds[0] = b[0];
        if (bounds[1] > b[1]) bounds[1] = b[1];
        if (bounds[2] < b[2]) bounds[2] = b[2];
        if (bounds[3] < b[3]) bounds[3] = b[3];
    }
}

SVGItemPtr loadSVG(const std::string& filepath, double dpi)
{
    NSVGimage* image = nsvgParseFromFile(filepath.c_str(), "px", dpi);
    if (!image) {
        std::cerr << "Could not open SVG image." << std::endl;
        return nullptr;
    }

    std::vector<SVGShapePtr> shapes = loadSVGSub(image);

    nsvgDelete(image);

    SVGItemPtr item = std::make_shared<SVGItem>();
    item->setSVGShapes(shapes);

    return item;
}

SVGItemPtr loadSVGFronString(const std::string& text, double dpi)
{
    std::vector<char> buf(text.data(), text.data()+(text.size()+1));
    NSVGimage* image = nsvgParse(buf.data(), "px", dpi);

    if (!image) {
        std::cerr << "Could not open SVG image." << std::endl;
        return nullptr;
    }

    std::vector<SVGShapePtr> shapes = loadSVGSub(image);

    nsvgDelete(image);

    SVGItemPtr item = std::make_shared<SVGItem>();
    item->setSVGShapes(shapes);

    return item;
}

void renderSVG(ptgl::Renderer2D* r, const SVGShapeList& svgShapeList, bool setNvgBeginEndFrame)
{
    if (!r) return;

    NVGcontext* vg = r->isPickingMode() ? r->pickingNvgContext() : r->nvgContext();

    if (setNvgBeginEndFrame) {
        int width = r->windowWidth();
        int height = r->windowHeight();
        float pixelRatio = 1.0;
        nvgBeginFrame(vg, width, height, pixelRatio);
    }

    for (const auto& shape : svgShapeList) {
        renderSVGShape(r, shape, false);
    }

    if (setNvgBeginEndFrame) {
        nvgEndFrame(vg);
        r->shaderProgram()->bind();
    }
}

void renderSVGShape(ptgl::Renderer2D* r, SVGShapePtr svgShape, bool setNvgBeginEndFrame)
{
    if (!r || !svgShape) return;

    NVGcontext* vg = r->isPickingMode() ? r->pickingNvgContext() : r->nvgContext();

    if (setNvgBeginEndFrame) {
        int width = r->windowWidth();
        int height = r->windowHeight();
        float pixelRatio = 1.0;
        nvgBeginFrame(vg, width, height, pixelRatio);
    }

    r->setStrokeColor(svgShape->strokeColor);
    r->setFillColor(svgShape->fillColor);
    renderSVGShapeSub(r, svgShape, false);

    if (setNvgBeginEndFrame) {
        nvgEndFrame(vg);
        r->shaderProgram()->bind();
    }
}

// https://github.com/memononen/nanosvg/issues/58
void renderSVGShapeSub(ptgl::Renderer2D* r, SVGShapePtr svgShape, bool setNvgBeginEndFrame)
{
    if (!r || !svgShape) return;

    auto shape = svgShape;
    NVGcontext* vg = r->isPickingMode() ? r->pickingNvgContext() : r->nvgContext();
    const auto& fillColor = r->isPickingMode() ? r->pickColor() : r->fillColor();
    const auto& strokeColor = r->isPickingMode() ? r->pickColor() : r->strokeColor();

    if (setNvgBeginEndFrame) {
        int width = r->windowWidth();
        int height = r->windowHeight();
        float pixelRatio = 1.0;
        nvgBeginFrame(vg, width, height, pixelRatio);
    }

    nvgFillColor(vg, NVGcolor{(float)fillColor[0], (float)fillColor[1], (float)fillColor[2], (float)fillColor[3]});
    nvgStrokeColor(vg, NVGcolor{(float)strokeColor[0], (float)strokeColor[1], (float)strokeColor[2], (float)strokeColor[3]});

    nvgStrokeWidth(vg, shape->strokeWidth);

    Eigen::Vector3d translate = r->transformStack().getTranslation();
//    Eigen::Matrix3d rotate = r->transformStack().getRotation();
    Eigen::Vector3d scale = r->transformStack().getScale();

    nvgResetTransform(vg);

    nvgTranslate(vg, translate(0), translate(1));
    nvgScale(vg, scale(0), scale(1));

    for (auto& path : shape->pathList) {
        nvgBeginPath(vg);
        nvgMoveTo(vg, path.pts[0], path.pts[1]);

        int npts = path.pts.size()/2;
        for (int i = 0; i < npts - 1; i += 3) {
            float* p = &path.pts[i*2];
            nvgBezierTo(vg, p[2], p[3], p[4], p[5], p[6], p[7]);
        }

        if (path.closed) {
            nvgLineTo(vg, path.pts[0], path.pts[1]);
        }

        if (shape->fill) {
            nvgFill(vg);
        }

        if (shape->stroke) {
            nvgStroke(vg);
        }
    }

    if (setNvgBeginEndFrame) {
        nvgEndFrame(vg);
        r->shaderProgram()->bind();
    }
}

}
} /* namespace ptgl */
