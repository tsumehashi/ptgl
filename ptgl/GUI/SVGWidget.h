#ifndef PTGL_GUI_SVGWIDGET_H_
#define PTGL_GUI_SVGWIDGET_H_

#include <functional>
#include "ptgl/SVG/SVGGraphics2DItem.h"
#include "Widget.h"

namespace ptgl {
namespace gui {

class SVGWidget;
typedef std::shared_ptr<SVGWidget> SVGWidgetPtr;

class SVGWidget : public Widget {
public:
    SVGWidget();
    virtual ~SVGWidget();

    virtual void setWidth(int width) override;
    virtual void setHeight(int height) override;
    virtual void setSize(int width, int height) override;

    void setSpacing(int sx, int sy);
    void setSVGItem(ptgl::svg::SVGItemPtr item);
    void setSVGFilepath(const std::string& filepath);
    void setSVGFronString(const std::string& text);

    ptgl::svg::SVGGraphics2DItemPtr svgGraphics2DItem() { return svgGraphics2DItem_; }

    void setClickedFillColor(const std::string& name, const std::array<double, 4>& c);
    void setPickable(const std::string& name, bool on);

    void setChecked(const std::string&, bool checked);

    void setOnCheckedFunction(std::function<void (const std::string&, bool)> func) { onCheckedFunc_ = func; }

protected:

    virtual void render2DScene(ptgl::Renderer2D* r) override;
    virtual void postProcess() override;

    class SVGWidgetSVGSubGraphics2DItem : public ptgl::svg::SVGSubGraphics2DItem {
    public:
        SVGWidgetSVGSubGraphics2DItem() {
            clickedFillColor_ = {0.0, 1.0, 0.0, 1.0};
            hoverdFillColor_ = {0.0, 1.0, 0.5, 1.0};
        }
        virtual ~SVGWidgetSVGSubGraphics2DItem() {};

        void setClickedFillColor(const std::array<double, 4>& c) { clickedFillColor_ = c; }
        void setHoverdFillColor(const std::array<double, 4>& c) { hoverdFillColor_ = c; }
    private:
        virtual void render2DScene(ptgl::Renderer2D* r) override;

        std::array<double, 4> clickedFillColor_;
        std::array<double, 4> hoverdFillColor_;
    };

    ptgl::svg::SVGGraphics2DItemPtr svgGraphics2DItem_;

    std::function<void (const std::string&, bool)> onCheckedFunc_;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_SVGWIDGET_H_ */
