#ifndef PTGL_SVG_SVGGRAPHICS2DITEM_H_
#define PTGL_SVG_SVGGRAPHICS2DITEM_H_

#include "ptgl/Core/Graphics2DItem.h"
#include "SVGItem.h"

namespace ptgl {
namespace svg {

class SVGSubGraphics2DItem;
typedef std::shared_ptr<SVGSubGraphics2DItem> SVGSubGraphics2DItemPtr;

class SVGGraphics2DItem;
typedef std::shared_ptr<SVGGraphics2DItem> SVGGraphics2DItemPtr;

class SVGSubGraphics2DItemFactory;
typedef std::shared_ptr<SVGSubGraphics2DItemFactory> SVGSubGraphics2DItemFactoryPtr;

class SVGGraphics2DItem : public ptgl::Graphics2DItem {
public:
    SVGGraphics2DItem();
    virtual ~SVGGraphics2DItem();

    void setSVGSubGraphics2DItemFactory(SVGSubGraphics2DItemFactoryPtr factory) { factory_ = factory; }

    void setDrawSVGBounds(bool draw) { drawSVGBounds_ = draw; }
    void setSVGItem(SVGItemPtr item);

    virtual void setWidth(int width) override;    // keep aspect
    virtual void setHeight(int height) override;    // keep aspect
    virtual void setSize(int width, int height) override;

    void setSpacing(int sx, int sy);

    float spaceingX() const { return spaceX_; }
    float spaceingY() const { return spaceY_; }

    float scaleX() const { return scaleX_; }
    float scaleY() const { return scaleY_; }

    float offsetX() const { return offsetX_; }
    float offsetY() const { return offsetY_; }

    // require before addGraphics2DItem
    void initializeSVGShapes();

    const std::vector<SVGSubGraphics2DItemPtr>& getSubItems() const { return subItems_; }
    SVGSubGraphics2DItemPtr getSubItem(const std::string& name);

protected:
    void resizeSVG();

    // render scene
    virtual void render2DScene(ptgl::Renderer2D* r) override;

    virtual void postProcess() override;

    SVGSubGraphics2DItemFactoryPtr factory_;
    bool drawSVGBounds_ = false;
    SVGItemPtr svgItem_;

    std::vector<SVGSubGraphics2DItemPtr> subItems_;

    struct SVGShapeGroupInfo {
        std::string name;
        SVGShapeList shapes;
    };
    std::vector<SVGShapeGroupInfo> shapeGroupInfos_;

    float scaleX_ = 1.0;
    float scaleY_ = 1.0;
    float offsetX_ = 0.f;
    float offsetY_ = 0.f;
    float spaceX_ = 0.0;
    float spaceY_ = 0.0;

    enum SetSizeType {
        NotSetSize,
        KeepAspectWidth,
        KeepAspectHeight,
        SetSize,
    };
    SetSizeType setSizeType_ = NotSetSize;
};

class SVGSubGraphics2DItem : public ptgl::Graphics2DItem {
public:
    SVGSubGraphics2DItem();
    virtual ~SVGSubGraphics2DItem();

    void setParentSVGGraphics2DItem(SVGGraphics2DItem* item);
    void setSVGShapes(SVGShapeList shapes);

    const SVGShapeList& getSvgShapes() const { return svgShapes_; }

protected:
    // render scene
    virtual void render2DScene(ptgl::Renderer2D* r) override;
    void setSVGItemPosition(ptgl::Renderer2D* r, SVGGraphics2DItem* item);

    SVGShapeList svgShapes_;

    SVGGraphics2DItem* parentSVGItem_ = nullptr;
};

class SVGSubGraphics2DItemFactory {
public:
    SVGSubGraphics2DItemFactory() {}
    virtual ~SVGSubGraphics2DItemFactory() {}

    virtual SVGSubGraphics2DItemPtr createSubGraphics2DItem() {
        SVGSubGraphics2DItemPtr subItem = std::make_shared<SVGSubGraphics2DItem>();
        return subItem;
    }
};

template<typename SVGSubGraphics2DItemType>
class TemplateSVGSubGraphics2DItemFactory : public SVGSubGraphics2DItemFactory {
public:
    TemplateSVGSubGraphics2DItemFactory() {}
    virtual ~TemplateSVGSubGraphics2DItemFactory() {}

    virtual SVGSubGraphics2DItemPtr createSubGraphics2DItem() {
        return std::make_shared<SVGSubGraphics2DItemType>();
    }
};

}
} /* namespace ptgl */

#endif /* PTGL_SVG_SVGGRAPHICS2DITEM_H_ */
