#include "ptgl/Core/Renderer2D.h"
#include "SVGWidget.h"

#include "thirdparty/Core/nanovg/src/nanovg.h"

namespace ptgl {
namespace gui {

SVGWidget::SVGWidget()
{
    svgGraphics2DItem_ = std::make_shared<ptgl::svg::SVGGraphics2DItem>();
    ptgl::svg::SVGSubGraphics2DItemFactoryPtr factory = std::make_shared<ptgl::svg::TemplateSVGSubGraphics2DItemFactory<SVGWidgetSVGSubGraphics2DItem>>();
    svgGraphics2DItem_->setSVGSubGraphics2DItemFactory(factory);

    svgGraphics2DItem_->setSpacing(5, 5);
}

SVGWidget::~SVGWidget()
{

}

void SVGWidget::setWidth(int width)
{
    Widget::setWidth(width);
    svgGraphics2DItem_->setWidth(width);
}

void SVGWidget::setHeight(int height)
{
    Widget::setHeight(height);
    svgGraphics2DItem_->setHeight(height);
}

void SVGWidget::setSize(int width, int height)
{
    Widget::setSize(width, height);
    svgGraphics2DItem_->setSize(width, height);
}

void SVGWidget::setSpacing(int sx, int sy)
{
    svgGraphics2DItem_->setSpacing(sx, sy);
}

void SVGWidget::setSVGItem(ptgl::svg::SVGItemPtr item)
{
    svgGraphics2DItem_->setSVGItem(item);
    svgGraphics2DItem_->initializeSVGShapes();

    width_ = svgGraphics2DItem_->width();
    height_ = svgGraphics2DItem_->height();

    this->addChild(svgGraphics2DItem_);
}

void SVGWidget::setSVGFilepath(const std::string& filepath)
{
    ptgl::svg::SVGItemPtr item = ptgl::svg::loadSVG(filepath);
    setSVGItem(item);
}

void SVGWidget::setSVGFronString(const std::string& text)
{
    ptgl::svg::SVGItemPtr item = ptgl::svg::loadSVGFronString(text);
    setSVGItem(item);
}

void SVGWidget::setClickedFillColor(const std::string& name, const std::array<double, 4>& c)
{
    if (auto subItem = std::dynamic_pointer_cast<SVGWidgetSVGSubGraphics2DItem>(svgGraphics2DItem_->getSubItem(name))) {
        subItem->setClickedFillColor(c);
    }
}

void SVGWidget::setPickable(const std::string& name, bool on)
{
    if (auto subItem = svgGraphics2DItem_->getSubItem(name)) {
        subItem->setPickable(on);
    }
}

void SVGWidget::setChecked(const std::string& name, bool checked)
{
    // emit
    if (auto subItem = svgGraphics2DItem_->getSubItem(name)) {
        subItem->setChecked(checked);
        if (subItem->isCheckChanged()) {
            // emit
            if (onCheckedFunc_) {
                onCheckedFunc_(subItem->name(), subItem->isChecked());
            }
        }
    }
}

void SVGWidget::render2DScene(ptgl::Renderer2D* /*r*/)
{
    updatePos();
}

void SVGWidget::postProcess()
{
    Widget::postProcess();

    for (auto& subItem : svgGraphics2DItem_->getSubItems()) {
        if (subItem->isCheckChanged()) {
            // emit
            if (onCheckedFunc_) {
                onCheckedFunc_(subItem->name(), subItem->isChecked());
            }
        }
    }
}

// --- SVGWidgetSVGSubGraphics2DItem ---
void SVGWidget::SVGWidgetSVGSubGraphics2DItem::render2DScene(ptgl::Renderer2D* r)
{
    r->setStrokeWeight(1);
    r->setPointSize(1);

    r->pushMatrix();
    setSVGItemPosition(r, parentSVGItem_);

    NVGcontext* vg = r->isPickingMode() ? r->pickingNvgContext() : r->nvgContext();

    // begin frame
    {
        int width = r->windowWidth();
        int height = r->windowHeight();
        float pixelRatio = 1.0;
        nvgBeginFrame(vg, width, height, pixelRatio);
    }

    for (auto& shape : svgShapes_) {

        if (this->isChecked()) {
            r->setStrokeColor(shape->strokeColor);
            r->setFillColor(clickedFillColor_);
        } else {
            r->setStrokeColor(shape->strokeColor);
            r->setFillColor(shape->fillColor);
        }
        renderSVGShapeSub(r, shape, false);
    }

    // end frame
    {
        nvgEndFrame(vg);
        if (r->shaderProgram()) {
            r->shaderProgram()->bind();
        }
    }

    r->popMatrix();
}

}
} /* namespace ptgl */
