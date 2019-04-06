#include "SVGGraphics2DItem.h"
#include <limits>
#include <cmath>
#include "ptgl/Core/Renderer2D.h"

namespace ptgl {
namespace svg {

SVGGraphics2DItem::SVGGraphics2DItem() {

}

SVGGraphics2DItem::~SVGGraphics2DItem() {

}

void SVGGraphics2DItem::setWidth(int width)
{
    width_ = width;
    setSizeType_ = KeepAspectWidth;

    resizeSVG();
}

void SVGGraphics2DItem::setHeight(int height)
{
    height_ = height;
    setSizeType_ = KeepAspectHeight;

    resizeSVG();
}

void SVGGraphics2DItem::setSize(int width, int height)
{
    width_ = width;
    height_ = height;
    setSizeType_ = SetSize;

    resizeSVG();
}

void SVGGraphics2DItem::setSpacing(int sx, int sy)
{
    spaceX_ = sx;
    spaceY_ = sy;

    resizeSVG();
}

void SVGGraphics2DItem::setSVGItem(SVGItemPtr item)
{
    if (!item) return;

    svgItem_ = item;

    offsetX_ = svgItem_->bounds[0];
    offsetY_ = svgItem_->bounds[1];

    resizeSVG();
}

SVGSubGraphics2DItemPtr SVGGraphics2DItem::getSubItem(const std::string& name)
{

    for (auto& ptr : subItems_) {
        if (ptr->name() == name) {
            return ptr;
        }
    }
    return nullptr;
}

void SVGGraphics2DItem::initializeSVGShapes()
{
    if (!svgItem_) return;

    auto& svgShapeList = svgItem_->shapes;

    resizeSVG();

#if 0    // in the case of each path
    for (auto& shape : svgShapeList_) {
        SVGSubGraphics2DItemPtr subItem = std::make_shared<SVGSubGraphics2DItem>();
        subItem->setName(shape->name);
        subItem->setParentSVGGraphics2DItem(this);
        subItem->setSVGShape(shape);
        subItems_.push_back(subItem);
        this->addChild(subItem);
    }
#endif

    if (!factory_) {
        // set default
        factory_ = std::make_shared<SVGSubGraphics2DItemFactory>();
    }

    std::vector<SVGShapeGroupInfo> shapeGroupInfos;
    shapeGroupInfos.push_back(SVGShapeGroupInfo());
    shapeGroupInfos.back().name = svgShapeList.front()->name;
    for (auto& shape : svgShapeList) {
        if (shapeGroupInfos.back().name != shape->name) {
            shapeGroupInfos.push_back(SVGShapeGroupInfo());
            shapeGroupInfos.back().name = shape->name;
        }

        auto& group = shapeGroupInfos.back();
        group.shapes.push_back(shape);
    }
    shapeGroupInfos_ = shapeGroupInfos;

    for (auto& group : shapeGroupInfos_) {
        SVGSubGraphics2DItemPtr subItem = factory_->createSubGraphics2DItem();
        subItem->setName(group.name);
        subItem->setParentSVGGraphics2DItem(this);
        subItem->setSVGShapes(group.shapes);
        subItems_.push_back(subItem);
        this->addChild(subItem);
    }
}

void SVGGraphics2DItem::resizeSVG()
{
    if (!svgItem_) return;

    auto& orgBounds = svgItem_->bounds;

    float scaleX = std::abs((width_ - 2*spaceX_)/(orgBounds[2] - orgBounds[0]));
    float scaleY = std::abs((height_ - 2*spaceY_)/(orgBounds[3] - orgBounds[1]));

    switch (setSizeType_) {
    case KeepAspectWidth:
        scaleX_ = scaleX;
        scaleY_ = scaleX;
        break;
    case KeepAspectHeight:
        scaleY_ = scaleY;
        scaleX_ = scaleY;
        break;
    case SetSize:
        scaleX_ = scaleX;
        scaleY_ = scaleY;
        break;
    default:
        scaleX_ = 1.0;
        scaleY_ = 1.0;
        break;
    }

    float orgWidth = std::abs(orgBounds[2] - orgBounds[0]);
    float orgHeight = std::abs(orgBounds[3] - orgBounds[1]);

    width_ = scaleX_ * orgWidth + 2*spaceX_;
    height_ = scaleY_ * orgHeight + 2*spaceY_;
}

void SVGGraphics2DItem::render2DScene(ptgl::Renderer2D* r)
{
    // update position
    updatePos();

    if (drawSVGBounds_) {
        r->setStrokeWeight(1);
        r->setStrokeColor(0.1,0.1,0.1,1);
        auto rectMode = r->rectMode();
        r->setRectMode(ptgl::Renderer2D::Mode::Corner);
        float ix = x();
        float iy = y();
        r->setFillColor(1,0,0,0.0);
        r->drawRect(ix, iy, width(), height());
        r->setRectMode(rectMode);    // restore
    }
}

void SVGGraphics2DItem::postProcess()
{
    bool checked = false;
    for (auto& item : subItems_) {
//        const auto& name = item->name();
        if (item->isPicked()) {
            // todo
        }

        if (item->isChecked()) {
            // todo
            checked = true;
        }

        if (item->isCheckChanged()) {

        }
    }

    if (!checked) {

    }
}

// ---- SVGSubGraphics2DItem ---

SVGSubGraphics2DItem::SVGSubGraphics2DItem()
{

}

SVGSubGraphics2DItem::~SVGSubGraphics2DItem()
{

}

void SVGSubGraphics2DItem::setParentSVGGraphics2DItem(SVGGraphics2DItem* item)
{
    parentSVGItem_ = item;
}

void SVGSubGraphics2DItem::setSVGShapes(SVGShapeList shapes)
{
    svgShapes_ = shapes;
}

void SVGSubGraphics2DItem::setSVGItemPosition(ptgl::Renderer2D* r, SVGGraphics2DItem* parentItem)
{
    if (parentItem) {
        r->translate(parentItem->x() + parentItem->spaceingX(), parentItem->y() + parentItem->spaceingY());
        r->scale(parentItem->scaleX(), parentItem->scaleY());
        r->translate(-parentItem->offsetX(), -parentItem->offsetY());
    }
}

void SVGSubGraphics2DItem::render2DScene(ptgl::Renderer2D* r)
{
    r->setStrokeWeight(1);
    r->setPointSize(1);

    r->pushMatrix();
    setSVGItemPosition(r, parentSVGItem_);

    renderSVG(r, svgShapes_, true);

    r->popMatrix();
}

}
} /* namespace ptgl */
