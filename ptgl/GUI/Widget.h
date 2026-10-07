#ifndef PTGL_GUI_WIDGET_H_
#define PTGL_GUI_WIDGET_H_

#include <array>
#include "ptgl/Core/Graphics2DItem.h"
#include "Theme.h"

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
    // All GUI mutations run on the view/event thread. Sizes below are logical pixels.
    void setTheme(const Theme &theme);
    const Theme &theme() const;
    void setFocusable(bool on)
    {
        focusable_ = on;
        setEnabledKeyEvent(on);
    }
    bool isFocusable() const { return focusable_; }
    void setClipChildren(bool on) { clipChildren_ = on; }
    void setPopup(bool on) { popup_ = on; }
    bool isPopup() const { return popup_; }
    bool isPopupLayer() const;
    Rect rect() const { return {x(), y(), width(), height()}; }
    Rect clipRect() const;
    virtual bool hitTest(int x, int y) const;
    virtual void layout() { updatePos(); }
    virtual std::array<int, 2> sizeHint() const;
    void setPreferredSize(int w, int h);
    void setMinimumSize(int w, int h);
    std::array<int, 2> minimumSize() const;
    void drawFrame(ptgl::Renderer2D *r, bool active = false) const;
    void drawCaption(ptgl::Renderer2D *r, const std::string &text, int inset = 6) const;
    void cancelInteraction() override;

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
  std::shared_ptr<const Theme> theme_;
  bool focusable_ = false, clipChildren_ = false;
  bool popup_ = false;
  std::array<int, 2> preferred_{{120, 28}}, minimum_{{0, 0}};

  // render
  virtual void renderTextScene(ptgl::TextRenderer *r) override;

  // Hover event
  virtual void hoverEnterEvent(ptgl::GraphicsItemHoverEvent *e) override;
  virtual void hoverLeaveEvent(ptgl::GraphicsItemHoverEvent *e) override;
  virtual void hoverMoveEvent(ptgl::GraphicsItemHoverEvent *e) override;

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
