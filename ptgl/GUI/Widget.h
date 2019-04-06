#ifndef PTGL_GUI_WIDGET_H_
#define PTGL_GUI_WIDGET_H_

#include <array>
#include "ptgl/Core/Graphics2DItem.h"

namespace ptgl {
namespace gui {

class Widget;
typedef std::shared_ptr<Widget> WidgetPtr;

class Widget : public Graphics2DItem {
public:
    Widget();
    virtual ~Widget();

    virtual void addWidget(WidgetPtr widget);
    virtual void removeWidget(WidgetPtr widget);

    // ToolTip
    virtual bool isEnabledToolTip() const { return enableToolTip_; }
    virtual void setEnableToolTip(bool on) { enableToolTip_ = on; }

    virtual double toolTipWaitShowTime() const { return toolTipWaitShowTime_; }
    virtual void setToolTipWaitShowTime(double time) { toolTipWaitShowTime_ = time; }

    virtual const std::string& toolTipText() const { return toolTipText_; }
    virtual int toolTipTextSize() const { return tootTipTextSize_; }
    virtual const std::array<double, 4>& toolTipTextColor() const { return toolTipTextColor_; }
    virtual const std::array<double, 4>& toolTipBackgroundColor() const { return toolTipBackgroundColor_; }

    virtual void setToolTipText(const std::string& text) { toolTipText_ = text; }
    virtual void setToolTipSize(int textSize) { tootTipTextSize_ = textSize; }
    virtual void setToolTipTextColor(const std::array<double, 4>& rgba) { toolTipTextColor_ = rgba;    }
    virtual void setToolTipBackgroundColor(const std::array<double, 4>& rgba) { toolTipBackgroundColor_ = rgba;    }

protected:

    // render
    virtual void renderTextScene(ptgl::TextRenderer* r) override;

    // Hover event
    virtual void hoverEnterEvent(ptgl::GraphicsItemHoverEvent* e) override;
    virtual void hoverLeaveEvent(ptgl::GraphicsItemHoverEvent* e) override;
    virtual void hoverMoveEvent(ptgl::GraphicsItemHoverEvent* e) override;

    bool checkDrawToolTip();
    int toolTipLockX() const { return hoverLockX_; }
    int toolTipLockY() const { return hoverLockY_; }

    bool enableToolTip_ = false;
    int tootTipTextSize_;
    std::string toolTipText_;
    std::array<double, 4> toolTipTextColor_;
    std::array<double, 4> toolTipBackgroundColor_;

    bool drawToolTip_ = false;
    double toolTipWaitShowTime_;
    double hoverNotMovedTime_;
    int hoverLockX_ = 0;
    int hoverLockY_ = 0;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_WIDGET_H_ */
