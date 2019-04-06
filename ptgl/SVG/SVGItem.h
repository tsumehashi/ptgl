#ifndef PTGL_SVG_SVGITEM_H_
#define PTGL_SVG_SVGITEM_H_

#include <memory>
#include <vector>
#include <array>
#include <fstream>
#include "ptgl/Core/Vertex.h"

namespace ptgl {

class Renderer2D;

namespace svg {

struct SVGPath {
    bool closed = false;
    std::vector<float> pts;
};

struct SVGShape {
    std::string name;
    int index = 0;
    std::array<double, 4> bounds;
    std::array<double, 4> fillColor;      // rgba [0.0-1.0]
    std::array<double, 4> strokeColor;    // rgba [0.0-1.0]
    bool fill = false;
    bool stroke = false;
    double strokeWidth;
    std::vector<SVGPath> pathList;
};

typedef std::shared_ptr<SVGShape> SVGShapePtr;
typedef std::vector<SVGShapePtr> SVGShapeList;

class SVGItem {
public:
    SVGItem();
    SVGItem(const SVGShapeList& s);
    virtual ~SVGItem();

    SVGShapeList shapes;
    std::array<double, 4> bounds;    // x0,y0,x1,y1

    void setSVGShapes(const SVGShapeList& s);
};

typedef std::shared_ptr<SVGItem> SVGItemPtr;

SVGItemPtr loadSVG(const std::string& filepath, double dpi = 96.f);
SVGItemPtr loadSVGFronString(const std::string& text, double dpi = 96.f);

void renderSVG(ptgl::Renderer2D* r, const SVGShapeList& svgShapeList, bool setNvgBeginEndFrame = true);
void renderSVGShape(ptgl::Renderer2D* r, SVGShapePtr svgShape, bool setNvgBeginEndFrame = true);
void renderSVGShapeSub(ptgl::Renderer2D* r, SVGShapePtr svgShape, bool setNvgBeginEndFrame = true);

}
} /* namespace ptgl */

#endif /* PTGL_SVG_SVGITEM_H_ */
