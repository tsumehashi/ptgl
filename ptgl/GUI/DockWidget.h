#ifndef PTGL_GUI_DOCKWIDGET_H_
#define PTGL_GUI_DOCKWIDGET_H_

#include "Widget.h"
#include "PushButton.h"

namespace ptgl {
namespace gui {

class Renderer2D;
class TextRenderer;
class GraphicsItemMouseEvent;

class DockWidget;
typedef std::shared_ptr<DockWidget> DockWidgetPtr;

class DockWidget : public Widget {
public:
    DockWidget();
    DockWidget(const std::string& windowTitle);
    virtual ~DockWidget();

    void setWindowTitle(const std::string& windowTitle) { windowTitle_ = windowTitle; }
    const std::string& windowTitle() const { return windowTitle_; }

    void setEnableTitleBar(bool enable);
    bool isEnabledTitleBar() const { return enableTitleBar_; }

    virtual void clearWidgets();

    virtual void addWidget(WidgetPtr widget) override;
    virtual void removeWidget(WidgetPtr widget) override;

    size_t numWidgets() const { return addedWidgets_.size(); }

    bool isShowedWidgets() const { return showWidgets_; }
    virtual void titleBarButtonToggled(bool toggle);
    virtual void titleBarButtonPressed();

    void setEnableMove(bool on) { enableMove_ = on; }
    bool isEnabledMove() const { return enableMove_; }

    virtual void setPadding(int padding) { padding_ = padding; }
    int padding() const { return padding_; }

protected:

    class TitleBarButton : public PushButton {
    public:
        virtual void render2DScene(ptgl::Renderer2D* r) override;
        virtual void renderPicking2DScene(ptgl::Renderer2D* r)  override { render2DScene(r); }
    };

    void init();

    // render
    virtual void render2DScene(ptgl::Renderer2D* r) override;

    // Mouse event
    virtual void mouseMoveEvent(ptgl::GraphicsItemMouseEvent* e) override;

    std::string windowTitle_;

    std::vector<WidgetPtr> addedWidgets_;

    PushButtonPtr titleBarButton_;

    bool enableTitleBar_;
    bool showWidgets_;

    int titleBarHeight_;
    int padding_;

    bool enableMove_ = true;
};

}
} /* namespace ptgl */

#endif /* PTGL_GUI_DOCKWIDGET_H_ */
